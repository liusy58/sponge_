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