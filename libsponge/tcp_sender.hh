#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>
#include <utility>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    class Fin_seq{
      public:
        bool _is_valid;
        uint64_t _seq_no;
        Fin_seq(bool is_valid,uint64_t seq):_is_valid(is_valid),_seq_no(seq){}
    };

    class Timer{
      private:
        unsigned int _initial_retransmission_timeout;
        unsigned int _current_retransmission_timeout;
        unsigned int _time_elapsed;
        bool _start{false};
        bool _can_double{true};
      public:
        Timer(unsigned int retx_timeout):
            _initial_retransmission_timeout(retx_timeout),
            _current_retransmission_timeout(retx_timeout),
            _time_elapsed(0){}
        void start(){
            _start = true;
        }
        bool is_start(){
            return _start;
        }
        bool is_expired(){
            return _time_elapsed >= _current_retransmission_timeout;
        }
        void timer_restart(){
            _time_elapsed = 0;
        }
        void _reset_rto(){
            _current_retransmission_timeout = _initial_retransmission_timeout;
        }
        void close(){
            _start = false;
        }
        void add_time(unsigned int _ticks){
            _time_elapsed += _ticks;
        }
        void start_if_not(){
            if(_start){
                return;
            }
            _start = true;
            _time_elapsed = 0;
        }
        void double_rto(){
            if(!_can_double){
                return;
            }
            _current_retransmission_timeout*=2;
        }
        void set_can_double(bool is){
            _can_double = is;
        }
    };

    class Data{
        uint64_t _seq_no;
        TCPSegment _seg;

      public:
        Data(uint64_t seq_no,TCPSegment seg):_seq_no(seq_no),_seg(seg){};
        TCPSegment get_seg(){
            return _seg;
        }

        uint64_t get_seq_no(){
            return _seq_no;
        }
    };
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    Timer _timer;
    size_t _rev_win{0};
    std::deque<Data> _segment_in_flight{};
    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0}; //
    uint64_t _last_rev_seqno{static_cast<uint64_t>(-1)};
    uint64_t _bytes_in_flight{0}; //
    unsigned int _consecutive_retransmissions{0};// the counter of consecutive retransmmision
    bool _send_zero_win{false};
    bool _zero_win_handled{false};
    Fin_seq _fin_seq{false,0};
    bool _fin_send{false};
    void reset_consecutive_retransmission(){
        _consecutive_retransmissions = 0;
    }

    void update_flights_in_flight(const WrappingInt32 ackno);

    void handle_zero_win();
    void check_fin();
    void resend_fin_segment();
    void send_fin_segment();
    void retransmit();


  public:
    enum TCPSenderState{
        ERROR = 0,
        CLOSED = 1,
        SYN_SENT = 2,
        SYN_ACKED = 3,
        FIN_SENT = 4,
        FIN_ACKED = 5
    };
    TCPSenderState _status{TCPSenderState::CLOSED};
    void set_status(TCPSenderState status);
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
    void send_totally_empty_seg();
    bool is_full_acked()const{
        return !_bytes_in_flight;
    }
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
