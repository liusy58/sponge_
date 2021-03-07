#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    ,_timer(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const {
    return _bytes_in_flight;
}

void TCPSender::fill_window() {
    switch (state_summary()) {
        case TCPSenderState::ERROR:{
            //std::cerr<<"In state Error but call fill_window "<<std::endl;
            break;
        }
        case TCPSenderState::CLOSED:{
            send_syn_segment();
            break;
        }
        case TCPSenderState::SYN_SENT:{
            break;
        }
        case TCPSenderState::SYN_ACKED1:{
            read_from_stream_to_segments();
            break;
        }
        case TCPSenderState::SYN_ACKED2:{
            send_fin_segment();
            break;
        }
        case TCPSenderState::FIN_SENT:{
            //std::cerr<<"TCPReceiver ::In state FIN_ACKED but call fill_window so we need to resend the fin segment"<<std::endl;
            break;
        }
        case TCPSenderState::FIN_ACKED:{
           // std::cerr<<"TCPReceiver ::In state FIN_ACKED but call fill_window "<<std::endl;
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {

    _rev_win = window_size;
    if(!_rev_win){
        _rev_win = 1;
        _timer.set_can_double(false);
    }else{
        _timer.set_can_double(true);
    }
    update_flights_in_flight(ackno);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(!_timer.is_start()){
        return;
    }
    if(!_bytes_in_flight){
        _timer.close();
        return;
    }
    _timer.add_time(ms_since_last_tick);
    if(_timer.is_expired()&&_bytes_in_flight>0){
        retransmit();
        _timer.double_rto();
        _timer.timer_restart();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _consecutive_retransmissions;
}

void TCPSender::send_syn_segment(){
    TCPSegment seg;
    seg.header().seqno = wrap(0,_isn);
    seg.header().syn = true;
    _bytes_in_flight = 1;
    _next_seqno = 1;
    _segments_out.push(seg);
    _timer.start_if_not();
}

void TCPSender::resend_fin_segment() {

    TCPSegment seg;
    seg.header().fin = true;
    seg.header().seqno = wrap(_next_seqno-1,_isn);
    _segments_out.push(seg);
    _timer.start_if_not();
}

void TCPSender::send_fin_segment() {
    if(_rev_win<=0 || _rev_win <= _bytes_in_flight){
        return;
    }
    TCPSegment seg;
    seg.header().fin = true;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
    _bytes_in_flight += seg.length_in_sequence_space();
    _next_seqno += seg.length_in_sequence_space();
    _timer.start_if_not();
}

void TCPSender::retransmit() {
    _consecutive_retransmissions += 1;
    if(_consecutive_retransmissions > TCPConfig::MAX_RETX_ATTEMPTS){
        return;
    }
    switch (state_summary()){
        case TCPSenderState::ERROR:
        case TCPSenderState::CLOSED:
        case TCPSenderState::FIN_ACKED:{
            //std::cerr<<"TCPReceiver ::In state ERROR or CLOSED or FIN_ACKED but call retransmit"<<std::endl;
            break;
        }
        case TCPSenderState::SYN_SENT:{
            send_syn_segment();
            break;
        }
        case TCPSenderState::SYN_ACKED1:
        case TCPSenderState::SYN_ACKED2:
        case TCPSenderState::FIN_SENT:{
            if(_segment_in_flight.empty()){
                if(state_summary() == TCPSenderState::FIN_SENT){
                    resend_fin_segment();
                }
                break;
            }
            auto seg = _segment_in_flight.front().get_seg();
            _segments_out.push(seg);
            //if the timer is not running, start it runningso that
            // it will expire after RTO milliseconds
            _timer.start_if_not();
            break;
        }
    }
}

void TCPSender::update_flights_in_flight(const WrappingInt32 ackno){
    uint64_t seq_no = unwrap(ackno,_isn,_next_seqno);
    switch (state_summary()) {
        case TCPSenderState::ERROR:
        case TCPSenderState::CLOSED:
        case TCPSenderState::FIN_ACKED:{
           // std::cerr<<"TCPReceiver :: In state ERROR or CLOSED or FIN_ACKED"<<std::endl;
            break;
        }
        case TCPSenderState::SYN_SENT:{
            if(seq_no == 1){
                _last_rev_seqno = seq_no;
                _bytes_in_flight-=1;
            }
            break;
        }
        case TCPSenderState::SYN_ACKED1:
        case TCPSenderState::SYN_ACKED2:
        case TCPSenderState::FIN_SENT:{
            if(seq_no>_last_rev_seqno){
                _last_rev_seqno = seq_no;
                _timer._reset_rto();
                if(_bytes_in_flight){
                    _timer.timer_restart();
                }
                reset_consecutive_retransmission();
            }
            for(auto iter = _segment_in_flight.begin();iter!=_segment_in_flight.end();){
                auto _seq_no = iter->get_seq_no();
                auto _seg = iter->get_seg();
                if(_seq_no+_seg.length_in_sequence_space()<=seq_no){
                    _bytes_in_flight -= _seg.length_in_sequence_space();
                    iter = _segment_in_flight.erase(iter);
                }else{
                    iter++;
                }
            }
            if(seq_no == _next_seqno&&_bytes_in_flight==1){
                _bytes_in_flight -= 1;
            }
        }
    }
}

void TCPSender::send_totally_empty_seg(){
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}


void TCPSender::read_from_stream_to_segments(){
    while(_rev_win>0&&!_stream.buffer_empty()){
        auto len = min(_rev_win,TCPConfig::MAX_PAYLOAD_SIZE);
        auto str = _stream.read(len);
        len = str.size();
        TCPSegment seg;
        TCPHeader &header = seg.header();
        seg.payload() = Buffer(std::move(str));
        header.seqno = next_seqno();
        if(_rev_win>len&&_stream.eof()){
            header.fin = true;
        }
        header.seqno = next_seqno();
        _segment_in_flight.emplace_back(_next_seqno,seg);
        _segments_out.push(seg);
        _bytes_in_flight += seg.length_in_sequence_space();
        _rev_win -= seg.length_in_sequence_space();
        _next_seqno += seg.length_in_sequence_space();
        _timer.start_if_not();
    }

}