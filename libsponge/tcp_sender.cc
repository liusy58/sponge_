#include "tcp_sender.hh"

#include "tcp_config.hh"

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
    if(_status == TCPSenderState::FIN_ACKED){
        return;
    }
    if(_status == TCPSenderState::CLOSED){
        send_empty_segment();
        return;
    }
    if(_send_zero_win&&!_rev_win&&!_zero_win_handled){
        if(_stream.eof()){
            send_fin_segment();
            return;
        }
        handle_zero_win();
        return;
    }
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
            set_status(FIN_SENT);
        }
        header.seqno = next_seqno();
        _segment_in_flight.emplace_back(_next_seqno,seg);
        _segments_out.push(seg);
        _bytes_in_flight += seg.length_in_sequence_space();
        _rev_win -= seg.length_in_sequence_space();
        _next_seqno += seg.length_in_sequence_space();

        _timer.start_if_not();
    }
    check_fin();
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {

    _rev_win = window_size;
    _send_zero_win = (window_size ==0);
    _zero_win_handled = false;
    update_flights_in_flight(ackno);
    //When all outstanding data has been acknowledged, stop the retransmission timer.
//    if(!_bytes_in_flight){
//        _timer.close();
//    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(!_timer.is_start()){
        return;
    }
    _timer.add_time(ms_since_last_tick);

    if(_timer.is_expired()){
        retransmit();
        if(!_send_zero_win)
            _timer.double_rto();
        _timer.timer_restart();
    }

}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _consecutive_retransmissions;
}


void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(0,_isn);
    seg.header().syn = true;
    _bytes_in_flight = 1;
    _next_seqno = 1;
    _segments_out.push(seg);
    set_status(SYN_SENT);
    _timer.start_if_not();
}

// get the receiver's current status
void TCPSender::set_status(TCPSenderState status) {
//    if(_stream.error()){
//        _status = TCPSenderState::ERROR;
//    }else if(_next_seqno == 0){
//        _status = TCPSenderState::CLOSED;
//    }else if(_next_seqno == bytes_in_flight()){
//        _status = TCPSenderState::SYN_SENT;
//    }else if(_stream.eof()){
//        _status = TCPSenderState::SYN_ACKED;
//    }else if(_next_seqno < _stream.bytes_written() + 2){
//        _status = TCPSenderState::SYN_ACKED;
//    }else if(bytes_in_flight()){
//        _status = TCPSenderState::FIN_SENT;
//    }else{
//        _status = TCPSenderState::FIN_ACKED;
//    }
    _status = status;

}

void TCPSender::retransmit() {
    _consecutive_retransmissions += 1;
    if(_consecutive_retransmissions > TCPConfig::MAX_RETX_ATTEMPTS){
        return;
    }
    if(_status == TCPSenderState::SYN_SENT&&_bytes_in_flight==1){
        send_empty_segment();
        return;
    }
    if(_status == TCPSenderState::FIN_SENT&&_bytes_in_flight==1){
        resend_fin_segment();
        return;
    }

    if(_segment_in_flight.empty()){
        return;
    }
    /*
    * Retransmit the earliest (lowest sequence number) segment that hasn’t been fully
    acknowledged by the TCP receiver.
    */
    auto seg = _segment_in_flight.front().get_seg();
    _segments_out.push(seg);
    /*    if the timer is not running, start it running
     *    so that it will expire after RTO milliseconds
     */
    _timer.start_if_not();
}

void TCPSender::update_flights_in_flight(const WrappingInt32 ackno){
    uint64_t seq_no = unwrap(ackno,_isn,_next_seqno);
    if(_status == TCPSenderState::SYN_SENT){
        // handle wrong ackno
        if(seq_no >= 1){
            set_status(SYN_ACKED);
            _last_rev_seqno = seq_no;
            _bytes_in_flight-=1;
        }
    }else if(seq_no>_last_rev_seqno){
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
    if(seq_no == _next_seqno&&_status == FIN_SENT&&_bytes_in_flight==1){
        _bytes_in_flight -=1;
        set_status(FIN_ACKED);
    }

}

void TCPSender::handle_zero_win(){
    size_t len = 1;
    string str = _stream.read(len);
    TCPSegment seg;
    Buffer payload{static_cast<string &&>(str)};
    seg.payload() = payload;
    seg.header().seqno = next_seqno();
    _segment_in_flight.emplace_back(_next_seqno,seg);
    _segments_out.push(seg);
    _bytes_in_flight += 1;
    _next_seqno += 1;
    _timer.start_if_not();
    _zero_win_handled=true;
}

void TCPSender::check_fin(){
    if(!_rev_win||_rev_win<_bytes_in_flight){
        return;
    }
    if(_status==TCPSenderState::SYN_ACKED&&_stream.eof()){
        TCPSegment seg;
        seg.header().fin = true;
        seg.header().seqno = next_seqno();
        set_status(FIN_SENT);
        _fin_seq = Fin_seq(true,_next_seqno);
        _bytes_in_flight += seg.length_in_sequence_space();
        _next_seqno += seg.length_in_sequence_space();
        _segments_out.push(seg);
        set_status(FIN_SENT);
        _timer.start_if_not();
    }
}
void TCPSender::resend_fin_segment() {
    TCPSegment seg;
    seg.header().fin = true;
    seg.header().seqno = wrap(_fin_seq._seq_no,_isn);
    set_status(FIN_SENT);
    _segments_out.push(seg);
    _timer.start_if_not();
}

void TCPSender::send_fin_segment() {
    TCPSegment seg;
    seg.header().fin = true;
    seg.header().seqno = next_seqno();
    _fin_seq = Fin_seq(true,_next_seqno);
    set_status(FIN_SENT);
    _segments_out.push(seg);
    _bytes_in_flight += seg.length_in_sequence_space();
    _next_seqno += seg.length_in_sequence_space();
    _timer.start_if_not();
}