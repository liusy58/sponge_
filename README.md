# sponge_

## Lab0

### `webget`

```diff
diff --git a/apps/webget.cc b/apps/webget.cc
index 3b85ce3..288e272 100644
--- a/apps/webget.cc
+++ b/apps/webget.cc
@@ -6,6 +6,16 @@
 
 using namespace std;
 
+
+/*
+
+how to fetch a page?
+1. GET /hello HTTP/1.1
+2. Host: cs144.keithw.org
+3. Connection:  close
+4. \n\n
+*/
+
 void get_URL(const string &host, const string &path) {
     // Your code here.
 
@@ -17,8 +27,18 @@ void get_URL(const string &host, const string &path) {
     // (not just one call to read() -- everything) until you reach
     // the "eof" (end of file).
 
-    cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
-    cerr << "Warning: get_URL() has not been implemented yet.\n";
+//    cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
+//    cerr << "Warning: get_URL() has not been implemented yet.\n";
+    Address address(host,"http");
+    TCPSocket socket;
+    socket.connect(address);
+    string request="GET "+ path + " HTTP/1.1\r\n" + "HOST: " + host + "\r\n" +"Connection: close\r\n" + "\r\n\r\n";;
+    socket.write(request);
+    while(!socket.eof()){
+        auto reply = socket.read();
+        cout<<reply;
+    }
+
 }
```



> Attention: we need to change byte_read in `pop_output`

```diff
--- a/libsponge/byte_stream.cc
+++ b/libsponge/byte_stream.cc
@@ -9,45 +9,86 @@
 
 template <typename... Targs>
 void DUMMY_CODE(Targs &&... /* unused */) {}
-
+#include <iostream>
 using namespace std;
 
-ByteStream::ByteStream(const size_t capacity) { DUMMY_CODE(capacity); }
+ByteStream::ByteStream(const size_t capacity):_capacity(capacity), _input_ended(0),_bytes_read(0),_bytes_write(0),_eof(0){}
+
 
+// Write a string of bytes into the stream. 
+//Write as many as will fit, and return the number of bytes written.
 size_t ByteStream::write(const string &data) {
-    DUMMY_CODE(data);
-    return {};
+    auto len = min(data.size(),remaining_capacity());
+    size_t i=0;
+    for(auto c:data){
+        if(i>=len){
+            break;
+        }
+        _buffer.push_back(c);
+        ++i;
+    }
+    _bytes_write += len;
+    return len;
 }
 
 //! \param[in] len bytes will be copied from the output side of the buffer
 string ByteStream::peek_output(const size_t len) const {
-    DUMMY_CODE(len);
-    return {};
+    auto nbytes = min(len,_buffer.size());
+    return string(_buffer.begin(),_buffer.begin()+nbytes);
 }
 
 //! \param[in] len bytes will be removed from the output side of the buffer
-void ByteStream::pop_output(const size_t len) { DUMMY_CODE(len); }
+void ByteStream::pop_output(const size_t len) {
+    auto nbytes = min(len,_buffer.size());
+    _bytes_read += nbytes;
+    while(nbytes--){
+        _buffer.pop_front();
+    }
+    if(_input_ended&&_buffer.empty()){
+        _eof=1;
+    }
+ }
 
 //! Read (i.e., copy and then pop) the next "len" bytes of the stream
 //! \param[in] len bytes will be popped and returned
 //! \returns a string
 std::string ByteStream::read(const size_t len) {
-    DUMMY_CODE(len);
-    return {};
+    string str=peek_output(len);
+    pop_output(len);
+    return str;
 }
 
-void ByteStream::end_input() {}
+void ByteStream::end_input() {
+    _input_ended = true;
+    if(_buffer.empty()){
+        _eof=1;
+    }
+}
 
-bool ByteStream::input_ended() const { return {}; }
+bool ByteStream::input_ended() const { 
+    return _input_ended;
+ }
 
-size_t ByteStream::buffer_size() const { return {}; }
+size_t ByteStream::buffer_size() const { 
+    return _buffer.size();
+ }
 
-bool ByteStream::buffer_empty() const { return {}; }
+bool ByteStream::buffer_empty() const { 
+    return _buffer.empty();    
+ }
 
-bool ByteStream::eof() const { return false; }
+bool ByteStream::eof() const { 
+    return _eof;
+ }
 
-size_t ByteStream::bytes_written() const { return {}; }
+size_t ByteStream::bytes_written() const { 
+    return _bytes_write;
+ }
 
-size_t ByteStream::bytes_read() const { return {}; }
+size_t ByteStream::bytes_read() const { 
+    return _bytes_read;
+ }
 
-size_t ByteStream::remaining_capacity() const { return {}; }
+size_t ByteStream::remaining_capacity() const { 
+    return _capacity - _buffer.size();
+ }
diff --git a/libsponge/byte_stream.hh b/libsponge/byte_stream.hh
index 71317c2..a361826 100644
--- a/libsponge/byte_stream.hh
+++ b/libsponge/byte_stream.hh
@@ -2,6 +2,7 @@
 #define SPONGE_LIBSPONGE_BYTE_STREAM_HH
 
 #include <string>
+#include <deque>
 
 //! \brief An in-order byte stream.
 
@@ -18,6 +19,13 @@ class ByteStream {
     // different approaches.
 
     bool _error{};  //!< Flag indicating that the stream suffered an error.
+    std::deque<char>_buffer{};
+    size_t _capacity;
+    bool _input_ended;
+    size_t _bytes_read;
+    size_t _bytes_write;
+    bool _eof;
+
 
   public:
     //! Construct a stream with room for `capacity` bytes.

```




## Lab1

```diff
diff --git a/libsponge/stream_reassembler.cc b/libsponge/stream_reassembler.cc
index 988df9f..acb4a69 100644
--- a/libsponge/stream_reassembler.cc
+++ b/libsponge/stream_reassembler.cc
@@ -1,5 +1,6 @@
 #include "stream_reassembler.hh"
-
+#include <iostream>
+#include <algorithm>
 // Dummy implementation of a stream reassembler.
 
 // For Lab 1, please replace with a real implementation that passes the
@@ -12,15 +13,149 @@ void DUMMY_CODE(Targs &&... /* unused */) {}
 
 using namespace std;
 
-StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}
+StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),_unassembled_bytes(0),
+_first_unread(0),_eof(false),_last_acceptable(0) {}
 
 //! \details This function accepts a substring (aka a segment) of bytes,
 //! possibly out-of-order, from the logical stream, and assembles any newly
 //! contiguous substrings and writes them into the output stream in order.
 void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
-    DUMMY_CODE(data, index, eof);
+    if(eof){
+        _eof=1;
+        _last_acceptable = index +data.size();
+    }
+    // receive data that has already assembled
+    if(data.empty()||data.size()+index-1<_first_unread){
+        write2stream();
+        return;
+    }
+    size_t first_unacceptable = _first_unread + _capacity ;
+    if(index>=first_unacceptable){
+        write2stream();
+        //? what if eof?
+        return;
+    }
+
+    push2list(data,index);
+    write2stream();
 }
 
-size_t StreamReassembler::unassembled_bytes() const { return {}; }
+size_t StreamReassembler::unassembled_bytes() const { 
+    return _unassembled_bytes;
+ }
+
+
+//! \brief Is the internal state empty (other than the output stream)?
+//! \returns `true` if no substrings are waiting to be assembled
+bool StreamReassembler::empty() const { 
+    return _data_list.empty();
+ }
+
+// the data1 is the new data
+int StreamReassembler::type_overlap(Data data1,Data data2){
+    uint64_t start1 = data1._start_index;
+    uint64_t start2 = data2._start_index;
+    uint64_t end1 = data1._start_index + data1._data.size()-1;
+    uint64_t end2 = data2._start_index + data2._data.size()-1;
+    //no overlaping
+    if(end2<start1||start2>end1){
+        return 0;
+    }
+
+    //       [    ]
+    //    [     ]
+    // the new data contains the right part of the old
+    if(start1>start2&&end1>end2){
+        return 1;
+    }
+
+    //     [  ]
+    //   [     ]
+    // the new data is contained in the old
+    if(start1>start2&&end1<=end2){
+        return 2;
+    }
+
+    //     [   ]
+    //     [ ]
+    //the new data constans the old
+    if(start1<=start2&&end1>end2){
+        return 3;
+    }
 
-bool StreamReassembler::empty() const { return {}; }
+    //     [   ]
+    //     [    ]
+    // the new data contains the left part of the old
+    if(start1<=start2&&end1<=end2){
+        return 4;
+    }
+    return 5;
+}
+
+void StreamReassembler::push2list(const string &data, const size_t index){
+    string str = data;
+    size_t i = index;
+    if(index < _first_unread){
+        size_t len = _first_unread - index;
+        str=str.substr(len);
+        i = _first_unread;
+    }
+    Data new_data(i,str);
+    for(auto iter = _data_list.begin();iter!=_data_list.end();){
+        int type = type_overlap(new_data,*iter);
+        if(!type){
+            iter++;
+        }else if(type == 1){
+            auto &_index = iter->_start_index;
+            auto &_data = iter->_data;
+            size_t len = _index + _data.size() - index;
+            _unassembled_bytes -= len;
+            _data = _data.substr(0,_data.size()-len);
+            iter++;
+        }else if(type == 2){
+            auto &_index = iter->_start_index;
+            auto &_data = iter->_data;
+            _data.replace(_data.begin()+index - _index,_data.begin()+index - _index+data.size(),data.begin(),data.end());
+            return;
+        }else if(type == 3){
+            _unassembled_bytes -= iter->_data.size();
+            iter = _data_list.erase(iter);
+        }else if (type == 4){
+            auto &_index = iter->_start_index;
+            auto &_data = iter->_data;
+            size_t len = index + data.size() - _index;
+            _unassembled_bytes -= len;
+            _data = _data.substr(len);
+            _index += len;
+        }
+
+    }
+    
+    _data_list.push_back(new_data);
+    _unassembled_bytes += new_data._data.size();
+
+}
+void StreamReassembler::write2stream(){
+    std::sort(_data_list.begin(),_data_list.end(),[](Data&d1,Data&d2){return d1._start_index<d2._start_index;});
+    for(auto iter = _data_list.begin();iter!=_data_list.end()&&_output.remaining_capacity()>0;){
+        auto &_index = iter->_start_index;
+        auto &_data = iter->_data;
+        if(_index == _first_unread){
+            auto bytes = _output.write(_data);
+            if(bytes != _data.size()){
+                _index += bytes;
+                _data = _data.substr(bytes);
+            }else{
+                iter = _data_list.erase(iter);
+            }
+            _unassembled_bytes -= bytes;
+            _first_unread += bytes;
+        }else{
+            iter++;
+        }
+    }
+    if(_eof&&_last_acceptable == _first_unread&&_data_list.empty()){
+        _output.end_input();
+    }
+
+}
diff --git a/libsponge/stream_reassembler.hh b/libsponge/stream_reassembler.hh
index a14c1e2..9881755 100644
--- a/libsponge/stream_reassembler.hh
+++ b/libsponge/stream_reassembler.hh
@@ -5,16 +5,31 @@
 
 #include <cstdint>
 #include <string>
-
+#include <vector>
 //! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
 //! possibly overlapping) into an in-order byte stream.
 class StreamReassembler {
   private:
+    class Data{
+      public:
+        uint64_t _start_index;
+        std::string _data;
+        Data(uint64_t start_index,std::string data):_start_index(start_index),_data(data){}
+    };
+
     // Your code here -- add private members as necessary.
 
     ByteStream _output;  //!< The reassembled in-order byte stream
     size_t _capacity;    //!< The maximum number of bytes
-
+    size_t _unassembled_bytes;  //!< The number of bytes not assembled
+    uint64_t _first_unread;
+    bool _eof;
+    uint64_t _last_acceptable;
+    std::vector<Data> _data_list{};
+    // the data1 is the new data
+    int type_overlap(Data data1,Data data2);
+    void push2list(const std::string &data, const size_t index);
+    void write2stream();
   public:
     //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
     //! \note This capacity limits both the bytes that have been reassembled,
```


## Lab2

```diff
diff --git a/libsponge/stream_reassembler.hh b/libsponge/stream_reassembler.hh
index 9881755..d6f43e4 100644
--- a/libsponge/stream_reassembler.hh
+++ b/libsponge/stream_reassembler.hh
@@ -61,6 +61,14 @@ class StreamReassembler {
     //! \brief Is the internal state empty (other than the output stream)?
     //! \returns `true` if no substrings are waiting to be assembled
     bool empty() const;
+
+    uint64_t first_unread()const{
+      return _first_unread;
+    }
+    size_t window_size()const{
+      return _output.remaining_capacity();
+    }
 };
 
+
 #endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
diff --git a/libsponge/tcp_receiver.cc b/libsponge/tcp_receiver.cc
index 4f0ee81..f7ca65d 100644
--- a/libsponge/tcp_receiver.cc
+++ b/libsponge/tcp_receiver.cc
@@ -11,9 +11,45 @@ void DUMMY_CODE(Targs &&... /* unused */) {}
 using namespace std;
 
 void TCPReceiver::segment_received(const TCPSegment &seg) {
-    DUMMY_CODE(seg);
+    auto header = seg.header();
+    auto playload = seg.payload();
+    uint64_t index = 0;
+    if(header.syn){
+        _isn_recv = true;
+        _isn = header.seqno;
+    }else{
+        index = unwrap(header.seqno,_isn,_reassembler.first_unread()+1)-1;
+    }
+    if(!_isn_recv){
+        return;
+    }
+    if(header.fin){
+        _fin_rev=true;
+    }
+    _reassembler.push_substring(playload.copy(),index,header.fin);
 }
 
-optional<WrappingInt32> TCPReceiver::ackno() const { return {}; }
 
-size_t TCPReceiver::window_size() const { return {}; }
+//! This is the beginning of the receiver's window, or in other words, the sequence number
+//! of the first byte in the stream that the receiver hasn't received.
+optional<WrappingInt32> TCPReceiver::ackno() const { 
+    if(!_isn_recv){
+        return {};
+    }
+    WrappingInt32 res( _isn + _reassembler.first_unread() + _isn_recv + (_reassembler.empty()&&_fin_rev));
+    return res;
+ }
+
+//! \brief The window size that should be sent to the peer
+//!
+//! Operationally: the capacity minus the number of bytes that the
+//! TCPReceiver is holding in its byte stream (those that have been
+//! reassembled, but not consumed).
+//!
+//! Formally: the difference between (a) the sequence number of
+//! the first byte that falls after the window (and will not be
+//! accepted by the receiver) and (b) the sequence number of the
+//! beginning of the window (the ackno).
+size_t TCPReceiver::window_size() const { 
+    return _reassembler.window_size();
+ }
diff --git a/libsponge/tcp_receiver.hh b/libsponge/tcp_receiver.hh
index 0856a3f..736a633 100644
--- a/libsponge/tcp_receiver.hh
+++ b/libsponge/tcp_receiver.hh
@@ -19,7 +19,11 @@ class TCPReceiver {
 
     //! The maximum number of bytes we'll store.
     size_t _capacity;
-
+    bool _isn_recv{false};
+    WrappingInt32 _isn{0};
+    uint64_t _abseq{0};
+    uint64_t _checkpoint{0};
+    bool _fin_rev{false};
   public:
     //! \brief Construct a TCP receiver
     //!
diff --git a/libsponge/wrapping_integers.cc b/libsponge/wrapping_integers.cc
index 7176532..d0cbd9a 100644
--- a/libsponge/wrapping_integers.cc
+++ b/libsponge/wrapping_integers.cc
@@ -14,8 +14,7 @@ using namespace std;
 //! \param n The input absolute 64-bit sequence number
 //! \param isn The initial sequence number
 WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
-    DUMMY_CODE(n, isn);
-    return WrappingInt32{0};
+    return isn+n;
 }
 
 //! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
@@ -29,6 +28,23 @@ WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
 //! and the other stream runs from the remote TCPSender to the local TCPReceiver and
 //! has a different ISN.
 uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
-    DUMMY_CODE(n, isn, checkpoint);
-    return {};
+    WrappingInt32 check=wrap(checkpoint,isn);
+
+    //   -------check-----------------n---------
+    if(check<n){
+        uint32_t diff1= n-check;
+        uint32_t diff2 = (1ul << 32) - diff1;
+        if(diff1<diff2||checkpoint<diff2){
+            return checkpoint + diff1;
+        }
+        return checkpoint - diff2;
+    }
+
+    //   -------n-----------------check---------
+    uint32_t diff1 = check -n;
+    uint32_t diff2 = (1ul << 32) - diff1;
+    if(diff2<diff1||checkpoint<diff1){
+        return checkpoint + diff2;
+    }
+    return checkpoint - diff1;
 }
diff --git a/libsponge/wrapping_integers.hh b/libsponge/wrapping_integers.hh
index 211a4e1..ce3b456 100644
--- a/libsponge/wrapping_integers.hh
+++ b/libsponge/wrapping_integers.hh
@@ -60,6 +60,11 @@ inline WrappingInt32 operator+(WrappingInt32 a, uint32_t b) { return WrappingInt
 
 //! \brief The point `b` steps before `a`.
 inline WrappingInt32 operator-(WrappingInt32 a, uint32_t b) { return a + -b; }
+
+inline bool operator>(WrappingInt32 a, WrappingInt32 b) { return a.raw_value()>b.raw_value(); }
+
+inline bool operator<(WrappingInt32 a, WrappingInt32 b) { return a.raw_value()<b.raw_value(); }
 //!@}
+inline int32_t operator+(WrappingInt32 a, WrappingInt32 b) { return a.raw_value() + b.raw_value(); }
 
 #endif  // SPONGE_LIBSPONGE_WRAPPING_INTEGERS_HH

```


## Lab3

```diff
diff --git a/libsponge/tcp_sender.cc b/libsponge/tcp_sender.cc
index 30ca8e4..5943a48 100644
--- a/libsponge/tcp_sender.cc
+++ b/libsponge/tcp_sender.cc
@@ -3,7 +3,7 @@
 #include "tcp_config.hh"
 
 #include <random>
-
+#include <algorithm>
 // Dummy implementation of a TCP sender
 
 // For Lab 3, please replace with a real implementation that passes the
@@ -20,19 +20,147 @@ using namespace std;
 TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
     : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
     , _initial_retransmission_timeout{retx_timeout}
-    , _stream(capacity) {}
+    , _stream(capacity)
+    , _timer(retx_timeout){}
 
-uint64_t TCPSender::bytes_in_flight() const { return {}; }
+uint64_t TCPSender::bytes_in_flight() const { 
+    return _bytes_in_flight;
+ }
 
-void TCPSender::fill_window() {}
+void TCPSender::fill_window() {
+    if(!_is_syn){
+        send_empty_segment();
+        return;
+    }
+    while(_win>0&&!_stream.buffer_empty()){
+        TCPSegment seg;
+        auto&header = seg.header();
+        auto&payload = seg.payload();
+        auto len = min(min(_win,_stream.buffer_size()),TCPConfig::MAX_PAYLOAD_SIZE);
+        string str = _stream.peek_output(len);
+        _stream.pop_output(len);
+        payload = Buffer(static_cast<string &&>(str));
+        header.seqno = next_seqno();
+//        if(_stream.buffer_empty()){
+//            header.fin = 1;
+//        }
+//        if(!_stream.bytes_read()){
+//            header.syn = 1;
+//        }
+        if(!_is_fin&&_stream.buffer_empty()&&_stream.input_ended()&&_win>len){
+            header.fin = 1;
+            _is_fin = 1 ;
+        }
+        _segment_in_flight.push_back(Data(_next_seqno,seg));
+        _segments_out.push(seg);
+        _next_seqno += seg.length_in_sequence_space();
+        _win -= seg.length_in_sequence_space();
+        _bytes_in_flight += seg.length_in_sequence_space();
+        _timer.start_if_not();
+    }
+    if(_win>_bytes_in_flight&&!_is_fin&&_stream.buffer_empty()&&_stream.input_ended()){
+        send_empty_with_flags(true,false);
+    }
+}
 
 //! \param ackno The remote receiver's ackno (acknowledgment number)
 //! \param window_size The remote receiver's advertised window size
-void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { DUMMY_CODE(ackno, window_size); }
+void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
+    if(!window_size){
+        _win=1;
+        _win_zero=1;
+    }
+    else{
+        if(window_size>_bytes_in_flight){
+            _win_zero=0;
+        }
+        _win = window_size;
+    }
+    update_seg_flight(ackno);
+    if(!_is_fin&&_stream.buffer_empty()&&_stream.input_ended()&&_win>_bytes_in_flight){
+        _is_fin=1;
+        send_empty_with_flags(true,false);
+    }
+ }
 
 //! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
-void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }
+void TCPSender::tick(const size_t ms_since_last_tick) { 
+    _timer.add_time(ms_since_last_tick);
+    if(_timer.is_expired()&&!_win_zero){
+        retransmit();
+        _timer.double_RTO();
+        _timer.reset();
+    }else if(_timer.is_expired()&&_win_zero){
+        retransmit();
+        _timer.reset();
+    }
+ }
+
+ //! \brief Number of consecutive retransmissions that have occurred in a row
+
+unsigned int TCPSender::consecutive_retransmissions() const {
+    return _consecutive_retranmission;
+ }
+
+//! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
+void TCPSender::send_empty_segment() {
+    TCPSegment seg;
+    auto&header = seg.header();
+    if(!_is_syn){
+        header.syn = 1;
+        _is_syn = 1;
+    }
+    header.seqno = wrap(_next_seqno,_isn);
+    _segment_in_flight.push_back(Data(_next_seqno,seg));
+    _next_seqno += seg.length_in_sequence_space();
+    _bytes_in_flight += seg.length_in_sequence_space();
+    _segments_out.push(seg);
+    _timer.start_if_not();
+}
+
+void TCPSender::update_seg_flight(const WrappingInt32 ackno){
+
+    auto ackno_unwrap = unwrap(ackno,_isn,_first_seqno_in_flight);
+    if(!_bytes_in_flight){
+        return;
+    }
+    for(auto iter=_segment_in_flight.begin();iter!=_segment_in_flight.end();){ 
+        if(iter->_is_received(ackno_unwrap)){
+            _first_seqno_in_flight = min(_first_seqno_in_flight,iter->get_seqno());
+            _bytes_in_flight -= iter->get_segment().length_in_sequence_space();
+            iter= _segment_in_flight.erase(iter);
+            _consecutive_retranmission = 0;
+            _timer.reset_RTO();
+            _timer.reset();
+
+        }else{
+            iter++;
+        }
+    }
+}
+
 
-unsigned int TCPSender::consecutive_retransmissions() const { return {}; }
+void TCPSender::retransmit(){
+    if(_consecutive_retranmission>TCPConfig::MAX_RETX_ATTEMPTS){
+        return;
+    }
+    if(_segment_in_flight.empty()){
+        return;
+    }
+    auto seg = _segment_in_flight[0].get_segment();
+    _segments_out.push(seg);
+    _consecutive_retranmission += 1;
+}
 
-void TCPSender::send_empty_segment() {}
+void TCPSender::send_empty_with_flags(bool fin, bool syn){
+    _is_fin=1;
+    TCPSegment seg;
+    seg.header().fin = fin;
+    seg.header().syn = syn;
+    seg.header().seqno =  wrap(_next_seqno,_isn);
+    seg.payload()=string("");
+    _segment_in_flight.push_back(Data(_next_seqno,seg));
+    _next_seqno += seg.length_in_sequence_space();
+    _bytes_in_flight += seg.length_in_sequence_space();
+    _segments_out.push(seg);
+}
\ No newline at end of file
diff --git a/libsponge/tcp_sender.hh b/libsponge/tcp_sender.hh
index ed0c4fb..63f77d7 100644
--- a/libsponge/tcp_sender.hh
+++ b/libsponge/tcp_sender.hh
@@ -16,7 +16,64 @@
 //! maintains the Retransmission Timer, and retransmits in-flight
 //! segments if the retransmission timer expires.
 class TCPSender {
+  enum STATE{CLOSED = 0,SYN_SENT,SYN_ACKED,FIN_SENT,FIN_ACKED};
   private:
+    class Data{
+      private: 
+        uint64_t absolute_seqno;
+        TCPSegment seg;
+
+      public:
+        Data(uint64_t _absolute_seqno,TCPSegment _seg):
+        absolute_seqno(_absolute_seqno),seg(_seg){}
+
+        bool _is_received(uint64_t seqno){
+          return ((absolute_seqno+seg.length_in_sequence_space())<=seqno);
+        }
+        uint64_t get_seqno(){
+          return absolute_seqno;
+        }
+        TCPSegment get_segment(){
+          return seg;
+        }
+    };
+    class Timer{
+      private:
+        bool start{false};
+        size_t time_elasped{0};
+        unsigned int initial_retransmission_timeout;
+        unsigned int retransmission_timeout;
+      public:
+        Timer(int retx_timeout):
+        initial_retransmission_timeout(retx_timeout)
+        , retransmission_timeout(retx_timeout){}
+        void add_time(const size_t ms_since_last_tick){
+          time_elasped += ms_since_last_tick;
+        }
+        bool is_expired(){
+          return time_elasped >= retransmission_timeout;
+        }
+        void double_RTO(){
+          retransmission_timeout*=2;
+        }
+        void reset_RTO(){
+          retransmission_timeout = initial_retransmission_timeout;
+        }
+        void reset(){
+          time_elasped = 0;
+        }
+        void start_if_not(){
+            if(start){
+                return;
+            }
+          start = true;
+          time_elasped = 0;
+          retransmission_timeout = initial_retransmission_timeout;
+        }
+        bool is_start(){
+          return start;
+        }
+    };
     //! our initial sequence number, the number for our SYN.
     WrappingInt32 _isn;
 
@@ -26,12 +83,25 @@ class TCPSender {
     //! retransmission timer for the connection
     unsigned int _initial_retransmission_timeout;
 
+
     //! outgoing stream of bytes that have not yet been sent
     ByteStream _stream;
 
+    Timer _timer;
+    std::vector<Data> _segment_in_flight{};
     //! the (absolute) sequence number for the next byte to be sent
     uint64_t _next_seqno{0};
-
+    bool _is_syn{false};
+    bool _is_fin{false};
+    size_t _win{0};
+    uint64_t _first_seqno_in_flight{0};
+    uint64_t _bytes_in_flight{0};
+    unsigned int _consecutive_retranmission{0};
+    bool _win_zero{false};
+    void update_seg_flight(const WrappingInt32 ackno);
+    void retransmit();
+
+    void send_empty_with_flags(bool fin, bool syn);
   public:
     //! Initialize a TCPSender
     TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,

```

## lab3 (v2)

```diff
diff --git a/libsponge/tcp_sender.cc b/libsponge/tcp_sender.cc
index 30ca8e4..4bec3f7 100644
--- a/libsponge/tcp_sender.cc
+++ b/libsponge/tcp_sender.cc
@@ -20,19 +20,232 @@ using namespace std;
 TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
     : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
     , _initial_retransmission_timeout{retx_timeout}
-    , _stream(capacity) {}
+    , _stream(capacity)
+    ,_timer(retx_timeout){}
 
-uint64_t TCPSender::bytes_in_flight() const { return {}; }
+uint64_t TCPSender::bytes_in_flight() const {
+    return _bytes_in_flight;
+}
 
-void TCPSender::fill_window() {}
+void TCPSender::fill_window() {
+    if(_status == TCPSenderState::FIN_ACKED){
+        return;
+    }
+    if(_status == TCPSenderState::CLOSED){
+        send_empty_segment();
+        return;
+    }
+    if(_send_zero_win&&!_rev_win&&!_zero_win_handled){
+        if(_stream.eof()){
+            send_fin_segment();
+            return;
+        }
+        handle_zero_win();
+        return;
+    }
+    while(_rev_win>0&&!_stream.buffer_empty()){
+        auto len = min(_rev_win,TCPConfig::MAX_PAYLOAD_SIZE);
+        auto str = _stream.read(len);
+        len = str.size();
+        TCPSegment seg;
+        TCPHeader &header = seg.header();
+        seg.payload() = Buffer(std::move(str));
+        header.seqno = next_seqno();
+        if(_rev_win>len&&_stream.eof()){
+            header.fin = true;
+            set_status(FIN_SENT);
+        }
+        header.seqno = next_seqno();
+        _segment_in_flight.emplace_back(_next_seqno,seg);
+        _segments_out.push(seg);
+        _bytes_in_flight += seg.length_in_sequence_space();
+        _rev_win -= seg.length_in_sequence_space();
+        _next_seqno += seg.length_in_sequence_space();
+
+        _timer.start_if_not();
+    }
+    check_fin();
+}
 
 //! \param ackno The remote receiver's ackno (acknowledgment number)
 //! \param window_size The remote receiver's advertised window size
-void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { DUMMY_CODE(ackno, window_size); }
+void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
+
+    _rev_win = window_size;
+    _send_zero_win = (window_size ==0);
+    _zero_win_handled = false;
+    update_flights_in_flight(ackno);
+    //When all outstanding data has been acknowledged, stop the retransmission timer.
+//    if(!_bytes_in_flight){
+//        _timer.close();
+//    }
+}
 
 //! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
-void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }
+void TCPSender::tick(const size_t ms_since_last_tick) {
+    if(!_timer.is_start()){
+        return;
+    }
+    _timer.add_time(ms_since_last_tick);
+
+    if(_timer.is_expired()){
+        retransmit();
+        if(!_send_zero_win)
+            _timer.double_rto();
+        _timer.timer_restart();
+    }
+
+}
+
+unsigned int TCPSender::consecutive_retransmissions() const {
+    return _consecutive_retransmissions;
+}
+
+
+void TCPSender::send_empty_segment() {
+    TCPSegment seg;
+    seg.header().seqno = wrap(0,_isn);
+    seg.header().syn = true;
+    _bytes_in_flight = 1;
+    _next_seqno = 1;
+    _segments_out.push(seg);
+    set_status(SYN_SENT);
+    _timer.start_if_not();
+}
+
+// get the receiver's current status
+void TCPSender::set_status(TCPSenderState status) {
+//    if(_stream.error()){
+//        _status = TCPSenderState::ERROR;
+//    }else if(_next_seqno == 0){
+//        _status = TCPSenderState::CLOSED;
+//    }else if(_next_seqno == bytes_in_flight()){
+//        _status = TCPSenderState::SYN_SENT;
+//    }else if(_stream.eof()){
+//        _status = TCPSenderState::SYN_ACKED;
+//    }else if(_next_seqno < _stream.bytes_written() + 2){
+//        _status = TCPSenderState::SYN_ACKED;
+//    }else if(bytes_in_flight()){
+//        _status = TCPSenderState::FIN_SENT;
+//    }else{
+//        _status = TCPSenderState::FIN_ACKED;
+//    }
+    _status = status;
+
+}
+
+void TCPSender::retransmit() {
+    _consecutive_retransmissions += 1;
+    if(_consecutive_retransmissions > TCPConfig::MAX_RETX_ATTEMPTS){
+        return;
+    }
+    if(_status == TCPSenderState::SYN_SENT&&_bytes_in_flight==1){
+        send_empty_segment();
+        return;
+    }
+    if(_status == TCPSenderState::FIN_SENT&&_bytes_in_flight==1){
+        resend_fin_segment();
+        return;
+    }
+
+    if(_segment_in_flight.empty()){
+        return;
+    }
+    /*
+    * Retransmit the earliest (lowest sequence number) segment that hasn’t been fully
+    acknowledged by the TCP receiver.
+    */
+    auto seg = _segment_in_flight.front().get_seg();
+    _segments_out.push(seg);
+    /*    if the timer is not running, start it running
+     *    so that it will expire after RTO milliseconds
+     */
+    _timer.start_if_not();
+}
+
+void TCPSender::update_flights_in_flight(const WrappingInt32 ackno){
+    uint64_t seq_no = unwrap(ackno,_isn,_next_seqno);
+    if(_status == TCPSenderState::SYN_SENT){
+        // handle wrong ackno
+        if(seq_no >= 1){
+            set_status(SYN_ACKED);
+            _last_rev_seqno = seq_no;
+            _bytes_in_flight-=1;
+        }
+    }else if(seq_no>_last_rev_seqno){
+        _last_rev_seqno = seq_no;
+        _timer._reset_rto();
+        if(_bytes_in_flight){
+            _timer.timer_restart();
+        }
+        reset_consecutive_retransmission();
+    }
+    for(auto iter = _segment_in_flight.begin();iter!=_segment_in_flight.end();){
+        auto _seq_no = iter->get_seq_no();
+        auto _seg = iter->get_seg();
+        if(_seq_no+_seg.length_in_sequence_space()<=seq_no){
+            _bytes_in_flight -= _seg.length_in_sequence_space();
+            iter = _segment_in_flight.erase(iter);
+        }else{
+            iter++;
+        }
+    }
+    if(seq_no == _next_seqno&&_status == FIN_SENT&&_bytes_in_flight==1){
+        _bytes_in_flight -=1;
+        set_status(FIN_ACKED);
+    }
+
+}
+
+void TCPSender::handle_zero_win(){
+    size_t len = 1;
+    string str = _stream.read(len);
+    TCPSegment seg;
+    Buffer payload{static_cast<string &&>(str)};
+    seg.payload() = payload;
+    seg.header().seqno = next_seqno();
+    _segment_in_flight.emplace_back(_next_seqno,seg);
+    _segments_out.push(seg);
+    _bytes_in_flight += 1;
+    _next_seqno += 1;
+    _timer.start_if_not();
+    _zero_win_handled=true;
+}
 
-unsigned int TCPSender::consecutive_retransmissions() const { return {}; }
+void TCPSender::check_fin(){
+    if(!_rev_win||_rev_win<_bytes_in_flight){
+        return;
+    }
+    if(_status==TCPSenderState::SYN_ACKED&&_stream.eof()){
+        TCPSegment seg;
+        seg.header().fin = true;
+        seg.header().seqno = next_seqno();
+        set_status(FIN_SENT);
+        _fin_seq = Fin_seq(true,_next_seqno);
+        _bytes_in_flight += seg.length_in_sequence_space();
+        _next_seqno += seg.length_in_sequence_space();
+        _segments_out.push(seg);
+        set_status(FIN_SENT);
+        _timer.start_if_not();
+    }
+}
+void TCPSender::resend_fin_segment() {
+    TCPSegment seg;
+    seg.header().fin = true;
+    seg.header().seqno = wrap(_fin_seq._seq_no,_isn);
+    set_status(FIN_SENT);
+    _segments_out.push(seg);
+    _timer.start_if_not();
+}
 
-void TCPSender::send_empty_segment() {}
+void TCPSender::send_fin_segment() {
+    TCPSegment seg;
+    seg.header().fin = true;
+    seg.header().seqno = next_seqno();
+    _fin_seq = Fin_seq(true,_next_seqno);
+    set_status(FIN_SENT);
+    _segments_out.push(seg);
+    _bytes_in_flight += seg.length_in_sequence_space();
+    _next_seqno += seg.length_in_sequence_space();
+    _timer.start_if_not();
+}
\ No newline at end of file
diff --git a/libsponge/tcp_sender.hh b/libsponge/tcp_sender.hh
index ed0c4fb..ea0a865 100644
--- a/libsponge/tcp_sender.hh
+++ b/libsponge/tcp_sender.hh
@@ -8,6 +8,7 @@
 
 #include <functional>
 #include <queue>
+#include <utility>
 
 //! \brief The "sender" part of a TCP implementation.
 
@@ -17,6 +18,78 @@
 //! segments if the retransmission timer expires.
 class TCPSender {
   private:
+    class Fin_seq{
+      public:
+        bool _is_valid;
+        uint64_t _seq_no;
+        Fin_seq(bool is_valid,uint64_t seq):_is_valid(is_valid),_seq_no(seq){}
+    };
+
+    class Timer{
+      private:
+        unsigned int _initial_retransmission_timeout;
+        unsigned int _current_retransmission_timeout;
+        unsigned int _time_elapsed;
+        bool _start{false};
+        bool _can_double{true};
+      public:
+        Timer(unsigned int retx_timeout):
+            _initial_retransmission_timeout(retx_timeout),
+            _current_retransmission_timeout(retx_timeout),
+            _time_elapsed(0){}
+        void start(){
+            _start = true;
+        }
+        bool is_start(){
+            return _start;
+        }
+        bool is_expired(){
+            return _time_elapsed >= _current_retransmission_timeout;
+        }
+        void timer_restart(){
+            _time_elapsed = 0;
+        }
+        void _reset_rto(){
+            _current_retransmission_timeout = _initial_retransmission_timeout;
+        }
+        void close(){
+            _start = false;
+        }
+        void add_time(unsigned int _ticks){
+            _time_elapsed += _ticks;
+        }
+        void start_if_not(){
+            if(_start){
+                return;
+            }
+            _start = true;
+            _time_elapsed = 0;
+        }
+        void double_rto(){
+            if(!_can_double){
+                return;
+            }
+            _current_retransmission_timeout*=2;
+        }
+        void set_can_double(bool is){
+            _can_double = is;
+        }
+    };
+
+    class Data{
+        uint64_t _seq_no;
+        TCPSegment _seg;
+
+      public:
+        Data(uint64_t seq_no,TCPSegment seg):_seq_no(seq_no),_seg(seg){};
+        TCPSegment get_seg(){
+            return _seg;
+        }
+
+        uint64_t get_seq_no(){
+            return _seq_no;
+        }
+    };
     //! our initial sequence number, the number for our SYN.
     WrappingInt32 _isn;
 
@@ -29,9 +102,37 @@ class TCPSender {
     //! outgoing stream of bytes that have not yet been sent
     ByteStream _stream;
 
+    Timer _timer;
+    size_t _rev_win{0};
+    std::deque<Data> _segment_in_flight{};
     //! the (absolute) sequence number for the next byte to be sent
-    uint64_t _next_seqno{0};
-
+    uint64_t _next_seqno{0}; //
+    uint64_t _last_rev_seqno{static_cast<uint64_t>(-1)};
+    uint64_t _bytes_in_flight{0}; //
+    unsigned int _consecutive_retransmissions{0};// the counter of consecutive retransmmision
+    bool _send_zero_win{false};
+    bool _zero_win_handled{false};
+    Fin_seq _fin_seq{false,0};
+    enum TCPSenderState{
+        ERROR = 0,
+        CLOSED = 1,
+        SYN_SENT = 2,
+        SYN_ACKED = 3,
+        FIN_SENT = 4,
+        FIN_ACKED = 5
+    };
+    TCPSenderState _status{TCPSenderState::CLOSED};
+    void set_status(TCPSenderState status);
+    void reset_consecutive_retransmission(){
+        _consecutive_retransmissions = 0;
+    }
+
+    void update_flights_in_flight(const WrappingInt32 ackno);
+
+    void handle_zero_win();
+    void check_fin();
+    void resend_fin_segment();
+    void send_fin_segment();
   public:
     //! Initialize a TCPSender
     TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
@@ -87,6 +188,8 @@ class TCPSender {
     //! \brief relative seqno for the next byte to be sent
     WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
     //!@}
+
+    void retransmit();
 };
 
```