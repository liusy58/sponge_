#ifndef SPONGE_LIBSPONGE_TCP_RECEIVER_HH
#define SPONGE_LIBSPONGE_TCP_RECEIVER_HH

#include "byte_stream.hh"
#include "stream_reassembler.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <optional>

//! \brief The "receiver" part of a TCP implementation.

//! Receives and reassembles segments into a ByteStream, and computes
//! the acknowledgment number and window size to advertise back to the
//! remote TCPSender.
class TCPReceiver {
    //! Our data structure for re-assembling bytes.
    StreamReassembler _reassembler;

    //! The maximum number of bytes we'll store.
    size_t _capacity;
    bool _isn_recv{false};
    std::optional<WrappingInt32> _isn{};
    uint64_t _abseq{0};
    uint64_t _checkpoint{0};
    bool _fin_rev{false};
    bool _rev_seg_out{false};

    enum class TCPReceiverStateSummary {
        ERROR,     // "error (connection was reset)";
        LISTEN,    // = "waiting for SYN: ackno is empty";
        SYN_RECV,  //= "SYN received (ackno exists), and input to stream hasn't ended";
        FIN_RECV   //= "input to stream has ended";
    };

  public:
    //! \brief Construct a TCP receiver
    //!
    //! \param capacity the maximum number of bytes that the receiver will
    //!                 store in its buffers at any give time.
    TCPReceiver(const size_t capacity) : _reassembler(capacity), _capacity(capacity) {}

    //! \name Accessors to provide feedback to the remote TCPSender
    //!@{

    //! \brief The ackno that should be sent to the peer
    //! \returns empty if no SYN has been received
    //!
    //! This is the beginning of the receiver's window, or in other words, the sequence number
    //! of the first byte in the stream that the receiver hasn't received.
    std::optional<WrappingInt32> ackno() const;

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
    size_t window_size() const;
    //!@}

    //! \brief number of bytes stored but not yet reassembled
    size_t unassembled_bytes() const { return _reassembler.unassembled_bytes(); }

    //! \brief handle an inbound segment
    void segment_received(const TCPSegment &seg);

    //! \name "Output" interface for the reader
    //!@{
    ByteStream &stream_out() { return _reassembler.stream_out(); }
    const ByteStream &stream_out() const { return _reassembler.stream_out(); }
    //!@}
    TCPReceiverStateSummary state_summary()const {
        if (stream_out().error()) {
            return TCPReceiverStateSummary::ERROR;
        } else if (not _isn.has_value()) {
            return TCPReceiverStateSummary::LISTEN;
        } else if (stream_out().input_ended()) {
            return TCPReceiverStateSummary::FIN_RECV;
        } else {
            return TCPReceiverStateSummary::SYN_RECV;
        }
    }
};

#endif  // SPONGE_LIBSPONGE_TCP_RECEIVER_HH
