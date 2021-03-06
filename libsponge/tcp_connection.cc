#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
const uint16_t MAX_WINSIZE = std::numeric_limits<uint16_t>::max();
size_t TCPConnection::remaining_outbound_capacity() const {
    return _receiver.window_size();
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
    // if the rst (reset) flag is set, sets both the inbound and outbound streams to the error
    // state and kills the connection permanently.
    if(seg.header().rst){
        handle_rst_rev();
    }else {
        // if the ack flag is set, tells the TCPSender about the fields it cares about on incoming
        // segments: ackno and window size.
        if(seg.header().ack){
            _sender.ack_received(seg.header().ackno,seg.header().win);
        }
        // gives the segment to the TCPReceiver so it can inspect the fields it cares about on
        // incoming segments: seqno, syn , payload, and fin .
        if(seg.header().fin){
            if(_sender._status != TCPSender::FIN_SENT&&_sender._status != TCPSender::FIN_ACKED){
                _remote_first_end = true;
            }
        }
        _receiver.segment_received(seg);
        //ack segment out of the window---should get an ACK
        if(!is_ack_seg(seg)&&seg.length_in_sequence_space()){
            fill_window(true);
        }
    }
    set_linger_after_streams_finish();
}
//! \brief Is the connection still alive in any way?
//! \returns `true` if either stream is still running or if the TCPConnection is lingering
//! after both streams have finished (e.g. to ACK retransmissions from the peer)
bool TCPConnection::active() const {
    if(_sender.stream_in().error()&&_receiver.stream_out().error()){
        return false;
    }
    //The inbound stream has been fully assembled and has ended.
    bool prereq1 = inbound_stream().eof();
    bool prereq2 = (_sender._status == TCPSender::FIN_SENT || _sender._status == TCPSender::FIN_ACKED);
    bool prereq3 = _sender.is_full_acked();
    if(prereq1&&prereq2&&prereq3&&!_linger_after_streams_finish){
        return false;
    }
    return true;
}
//! \brief Write data to the outbound byte stream, and send it over TCP if possible
//! \returns the number of bytes from `data` that were actually written.
size_t TCPConnection::write(const string &data) {
    fill_window(false);
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
    // the connection is only done after enough time (10 × cfg.rt timeout) has
    // elapsed since the last segment was received.
    if(_time_since_last_segment_received >= _cfg.rt_timeout*10 &&_receiver.stream_out().eof()&&_sender.stream_in().eof()){
        _linger_after_streams_finish = false;
    }
    if(_sender._status!=TCPSender::CLOSED)
        fill_window(false);
    set_linger_after_streams_finish();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    fill_window(true);
}

void TCPConnection::connect() {
    if(!is_state_valid({State::CLOSED})){
        std::cerr<<" In connect and the state is not correct "<<std::endl;
    }
    fill_window(true);
}

TCPConnection::~TCPConnection() {
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

void TCPConnection::handle_rst_rev(){
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
}

void TCPConnection::fill_window(bool create_empty) {
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS ){
        send_rst_seg();
        return;
    }
    _sender.fill_window();
    auto&ssegment_out = _sender.segments_out();
    if(ssegment_out.empty()&&create_empty) {
        _sender.send_totally_empty_seg();
    }
    while(!ssegment_out.empty()){
        auto seg = ssegment_out.front();
        ssegment_out.pop();
        if(_receiver.ackno().has_value()){
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        if(_receiver.window_size() > MAX_WINSIZE){
            seg.header().win=MAX_WINSIZE;
        }else{
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);
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
    TCPSegment seg;
    seg.header().rst = true;
    if(_receiver.ackno().has_value()){
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
    }
    _segments_out.push(seg);
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _linger_after_streams_finish = false;
}

