#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    auto header = seg.header();
    auto payload = seg.payload();
    uint64_t index = 0;
    if(header.syn){
        _isn_recv = true;
        _isn = header.seqno;
    }else{
        index = unwrap(header.seqno,_isn,_reassembler.first_unread()+1)-1;
        if(index+seg.payload().size()<=_reassembler.first_unread()){
            _rev_seg_out = true;
        }else{
            _rev_seg_out = false;
        }
    }
    if(!_isn_recv){
        return;
    }
    if(header.fin){
        _fin_rev=true;
    }
    _reassembler.push_substring(payload.copy(),index,header.fin);
}


//! This is the beginning of the receiver's window, or in other words, the sequence number
//! of the first byte in the stream that the receiver hasn't received.
optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_isn_recv){
        return {};
    }
    WrappingInt32 res( _isn + _reassembler.first_unread() + _isn_recv + (_reassembler.empty()&&_fin_rev));
    return res;
 }

//! \brief The window size that should be sent to the peer
//!
//! Operationally: the capacity minus the number of bytes that the
//! TCPReceiver is holding in its byte stream (those that have been
//! reassembled, but not consumed).
//!
//! Formally: the difference between (a) the sequence number of
//! the first byte that falls after the window (and will not be
//! accepted by the receiver) and (b) the sequence number of the
//! beginning of the window (the ackno).
size_t TCPReceiver::window_size() const { 
    return _reassembler.window_size();
 }


