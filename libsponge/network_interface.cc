#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway,
//! but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t
//! raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    if(_ip2eth.find(next_hop_ip) != _ip2eth.end()){
        EthernetFrame frame;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.payload() = dgram.serialize();
        frame.header().src = _ethernet_address;
        frame.header().dst = _ip2eth[next_hop_ip];
        _frames_out.push(frame);
    }else{
        if(_arp_in_flight.find(next_hop_ip)!=_arp_in_flight.end()){
            // avoid arp flood
            return;
        }else{
            EthernetFrame frame;
            frame.header().type = EthernetHeader::TYPE_ARP;
            frame.header().src = _ethernet_address;
            frame.header().dst = ETHERNET_BROADCAST;
            ARPMessage rep_arp;
            rep_arp.opcode = ARPMessage::OPCODE_REQUEST;
            rep_arp.sender_ip_address = _ip_address.ipv4_numeric();
            rep_arp.sender_ethernet_address = _ethernet_address;
            rep_arp.target_ip_address = next_hop_ip;
            frame.payload() = rep_arp.serialize();
            _frames_out.push(frame);
            _arp_in_flight.insert(next_hop_ip);
            _arp2tick[next_hop_ip] = 0;
            _waiting_datagrams.push_back({dgram,next_hop});
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    //cerr <<"in recv"<<endl;
    if(should_ignore(frame)){
        return {};
    }
    //cerr<<"here"<<endl;
    if(frame.header().type == EthernetHeader::TYPE_IPv4){
        InternetDatagram datagram;
        if(datagram.parse(frame.payload())==ParseResult::NoError){
            return datagram;
        }
        return {};
    }
    if(frame.header().type == EthernetHeader::TYPE_ARP){
        ARPMessage arp;
        if(arp.parse(frame.payload())==ParseResult::NoError){
            if(arp.opcode == ARPMessage::OPCODE_REQUEST){
                if(arp.target_ip_address != _ip_address.ipv4_numeric()){
                    return{};
                }
                ARPMessage rep_arp;
                rep_arp.opcode = ARPMessage::OPCODE_REPLY;
                rep_arp.sender_ip_address = _ip_address.ipv4_numeric();
                rep_arp.sender_ethernet_address = _ethernet_address;
                rep_arp.target_ip_address = arp.sender_ip_address;
                rep_arp.target_ethernet_address = arp.sender_ethernet_address;
                EthernetFrame new_frame;
                new_frame.payload() = rep_arp.serialize();
                new_frame.header().type = EthernetHeader::TYPE_ARP;
                new_frame.header().dst = arp.sender_ethernet_address;
                new_frame.header().src = _ethernet_address;
                _frames_out.push(new_frame);
                auto ip = arp.sender_ip_address;
                auto eth_address = arp.sender_ethernet_address;
                _ip2eth[ip] = eth_address;
                _ip2tick[ip] = 0 ;
            }else if(arp.opcode == ARPMessage::OPCODE_REPLY){
                auto ip = arp.sender_ip_address;
                auto eth_address = arp.sender_ethernet_address;
                cerr << "in arp rev the ip is " << ip << "the eth_address is "<<"\n";
                _ip2eth[ip] = eth_address;
                _ip2tick[ip] = 0 ;
                _arp_in_flight.erase(ip);
                update_waiting_datagra();
            }
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    for(auto iter = _ip2tick.begin();iter!=_ip2tick.end();){
        iter->second += ms_since_last_tick;
        if(iter->second>30000){
            _ip2eth.erase(iter->first);
            iter = _ip2tick.erase(iter);
        }else{
            iter++;
        }
    }
    for(auto iter = _arp2tick.begin();iter!=_arp2tick.end();){
        iter->second += ms_since_last_tick;
        if(iter->second>5000){
            _arp_in_flight.erase(iter->first);
            iter = _arp2tick.erase(iter);
        }else{
            iter++;
        }
    }
}



// You don’t want to flood the network with ARP requests. If the network
// interface already sent an ARP request about the same IP address in the last
// five seconds, don’t send a second request—just wait for a reply to the first one.
// Again, queue the datagram until you learn the destination Ethernet address.


bool NetworkInterface::should_ignore(const EthernetFrame &frame){
    if(frame.header().dst == ETHERNET_BROADCAST || frame.header().dst == _ethernet_address){
        return false;
    }
    return true;
}

void NetworkInterface::update_waiting_datagra() {
    int cnt = _waiting_datagrams.size();
    while(cnt--){
        auto seg = _waiting_datagrams.front();
        _waiting_datagrams.pop_front();
        send_datagram(seg._data_gram,seg._next_hop);
    }
}