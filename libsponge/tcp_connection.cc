#include "tcp_connection.hh"

#include <iostream>
#include <unistd.h>
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
const uint16_t MAX_WINSIZE = std::numeric_limits<uint16_t>::max();
void TCPConnection::printSeg(const TCPSegment &seg){
//    DUMMY_CODE(seg);
    ssr << "pid is "<< to_string(getpid()) << "    segFlags: " << "A: " << seg.header().ack << " " << "S: " << seg.header().syn << " " << "F: " << seg.header().fin << " " << "R: " << seg.header().rst ;
    ssr << "    header num: " << "seqno: " << seg.header().seqno.raw_value() << " " << "ack: " << seg.header().ackno.raw_value() << " " << "win: " << seg.header().win ;
    ssr  << "   payload size : " << seg.payload().copy().size() << "\n";
    cerr<<ssr.str();
}
size_t TCPConnection::remaining_outbound_capacity() const {
    return outbound_stream().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return _time_since_last_segment_received;
}

void TCPConnection::segment_received(const TCPSegment &seg) {

    _time_since_last_segment_received = 0;
    printSeg(seg);
    if(seg.header().rst){
        //rst flag is set, sets both the inbound and outbound streams
        // to the error
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        return;
    }else{
        if(seg.header().ack){
            _sender.ack_received(seg.header().ackno,seg.header().win);
        }
        _receiver.segment_received(seg);
        if(!is_ack_seg(seg)){
            fill_window(true);
        }else if(state_summary()!=State::LISTEN){
            //_sender.state_summary()!=TCPSender::TCPSenderState::CLOSED
            fill_window(false);
        }
//        string str = "connection state is " + to_string(state_summary())+"\n";
//        cerr<<str;
    }

}
//! \brief Is the connection still alive in any way?
//! \returns `true` if either stream is still running or if the TCPConnection is lingering
//! after both streams have finished (e.g. to ACK retransmissions from the peer)
bool TCPConnection::three_pre()const{
    bool prereq1 = inbound_stream().eof() && !_receiver.unassembled_bytes();
    bool prereq2 = (_sender.state_summary() == TCPSender::TCPSenderState::FIN_SENT)||(_sender.state_summary() == TCPSender::TCPSenderState::FIN_ACKED);

    bool prereq3 = _sender.is_full_acked();
    return prereq1&&prereq2&&prereq3;
}

bool TCPConnection::active() const {
    if(_sender.stream_in().error()&&_receiver.stream_out().error()){
        return false;
    }
    //The inbound stream has been fully assembled and has ended.

    if(three_pre()&&!_linger_after_streams_finish){
        return false;
    }
    return true;
}
//! \brief Write data to the outbound byte stream, and send it over TCP if possible
//! \returns the number of bytes from `data` that were actually written.
size_t TCPConnection::write(const string &data) {
    size_t sz = outbound_stream().write(data);
    fill_window(false);
    return sz;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received += ms_since_last_tick;
    if(_sender.consecutive_retransmissions()>TCPConfig::MAX_RETX_ATTEMPTS){
        send_rst_seg();
    }
    if(state_summary()!=State::LISTEN){
        fill_window(false);
    }
    set_linger_after_streams_finish();
    // the connection is only done after enough time (10 × cfg.rt timeout) has
    // elapsed since the last segment was received.
    if(_time_since_last_segment_received >= _cfg.rt_timeout*10 && three_pre()){
        _linger_after_streams_finish = false;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    if(_receiver.stream_out().input_ended()){
        _linger_after_streams_finish = false;
    }
    fill_window(false);
}

void TCPConnection::connect() {
    fill_window(true);
}

TCPConnection::~TCPConnection() {
    //myfile.close();
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            send_rst_seg();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}


void TCPConnection::fill_window(bool create_empty) {
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS ){
        send_rst_seg();
        return;
    }
    string str = "the pid is "+ to_string(getpid())+"in tcp_connection state is "+to_string(state_summary())+"\n";
    cerr << str;
    _sender.fill_window();
    auto&ssegment_out = _sender.segments_out();
    string er = "the pid is "+ to_string(getpid())+"the segment_out size is "+to_string(ssegment_out.size())+"\n";
    cerr<<er;
    if(ssegment_out.empty()&&create_empty) {
        _sender.send_totally_empty_seg();
    }

    while(!ssegment_out.empty()){
        er = "the pid is "+ to_string(getpid())+"the segment_out size is "+to_string(ssegment_out.size())+"\n";
        cerr<<er;
        auto seg = ssegment_out.front();
        if(_receiver.ackno().has_value()){
            str = "In ackno the state is " + to_string(_receiver.state_summary())+"\n";
            cerr<<str;
            string ss = "pid is "+to_string(getpid())+"the seg seqno is"+to_string(seg.header().seqno.raw_value())+"   the ackno is "+ to_string(_receiver.ackno().value().raw_value()) + "\n";
            cerr<<ss;
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();

        }else{
            er = "ackno has no value\n";
            cerr<<er;
        }
        if(_receiver.window_size() > MAX_WINSIZE){
            seg.header().win=MAX_WINSIZE;
        }else{
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);
        ssegment_out.pop();
    }
    set_linger_after_streams_finish();
}

bool TCPConnection::is_ack_seg(const TCPSegment&seg){
    auto header = seg.header();
    return (!header.urg&&!header.psh&&!header.rst&&!header.syn&&!header.fin&&header.ack&&seg.payload().size()==0);
}



// If the inbound stream ends before the TCPConnection
// has reached EOF on its outbound stream, this variable needs to be set to false.

void TCPConnection::set_linger_after_streams_finish(){
    if(inbound_stream().input_ended()&&!outbound_stream().eof()){
        _linger_after_streams_finish = false;
    }
}

void TCPConnection::send_rst_seg(){
    while(!_segments_out.empty()){
        _segments_out.pop();
    }
    _sender.send_totally_empty_seg();
    TCPSegment &seg = _sender.segments_out().back();
    seg.header().rst = true;
    _segments_out.push(seg);
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
}


