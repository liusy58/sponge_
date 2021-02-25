For build prereqs, see [the CS144 VM setup instructions](https://web.stanford.edu/class/cs144/vm_howto).

## Sponge quickstart

To set up your build directory:

	$ mkdir -p <path/to/sponge>/build
	$ cd <path/to/sponge>/build
	$ cmake ..

**Note:** all further commands listed below should be run from the `build` dir.

To build:

    $ make

You can use the `-j` switch to build in parallel, e.g.,

    $ make -j$(nproc)

To test (after building; make sure you've got the [build prereqs](https://web.stanford.edu/class/cs144/vm_howto) installed!)

    $ make check_lab0

or

	$ make check_lab1

etc.

The first time you run a `make check`, it may run `sudo` to configure two
[TUN](https://www.kernel.org/doc/Documentation/networking/tuntap.txt) devices for use during testing.

### build options

You can specify a different compiler when you run cmake:

    $ CC=clang CXX=clang++ cmake ..

You can also specify `CLANG_TIDY=` or `CLANG_FORMAT=` (see "other useful targets", below).

Sponge's build system supports several different build targets. By default, cmake chooses the `Release`
target, which enables the usual optimizations. The `Debug` target enables debugging and reduces the
level of optimization. To choose the `Debug` target:

    $ cmake .. -DCMAKE_BUILD_TYPE=Debug

The following targets are supported:

- `Release` - optimizations
- `Debug` - debug symbols and `-Og`
- `RelASan` - release build with [ASan](https://en.wikipedia.org/wiki/AddressSanitizer) and
  [UBSan](https://developers.redhat.com/blog/2014/10/16/gcc-undefined-behavior-sanitizer-ubsan/)
- `RelTSan` - release build with
  [ThreadSan](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/Thread_Sanitizer)
- `DebugASan` - debug build with ASan and UBSan
- `DebugTSan` - debug build with ThreadSan

Of course, you can combine all of the above, e.g.,

    $ CLANG_TIDY=clang-tidy-6.0 CXX=clang++-6.0 .. -DCMAKE_BUILD_TYPE=Debug

**Note:** if you want to change `CC`, `CXX`, `CLANG_TIDY`, or `CLANG_FORMAT`, you need to remove
`build/CMakeCache.txt` and re-run cmake. (This isn't necessary for `CMAKE_BUILD_TYPE`.)

### other useful targets

To generate documentation (you'll need `doxygen`; output will be in `build/doc/`):

    $ make doc

To lint (you'll need `clang-tidy`):

    $ make -j$(nproc) tidy

To run cppcheck (you'll need `cppcheck`):

    $ make cppcheck

To format (you'll need `clang-format`):

    $ make format

To see all available targets,

    $ make help


## Notes

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