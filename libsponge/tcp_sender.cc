#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

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
    , _rto(_initial_retransmission_timeout)
    {}

uint64_t TCPSender::bytes_in_flight() const { return { _bytes_in_flight }; }

void TCPSender::fill_window() {
    if (_sendsyn) {
        TCPSegment _seg;
        _seg.header().seqno = _isn;
        _seg.header().syn = true;
        _sendsyn = false;
        _synsent = true;
        send_segment(_seg);
        return ;
    }
    size_t _cansend;
    size_t win;
    if (_window_size == 0) {
        win = 1;
    }
    else win = _window_size;
    if (win < _next_seqno - _ackno)
        _cansend = 0;
    else _cansend = win - (_next_seqno - _ackno);
    while (_cansend > 0 && !_finsent) {
        string _payload = _stream.read(min(_cansend, TCPConfig::MAX_PAYLOAD_SIZE));
        TCPSegment _seg;
        _seg.payload() = Buffer(std::move(_payload));
        _seg.header().seqno = wrap(_next_seqno, _isn);
        if (_stream.eof() && _seg.length_in_sequence_space() < _cansend) {
            _seg.header().fin = true;
            _finsent = true;
        }
        if (_seg.length_in_sequence_space() == 0) {
            return ;
        }
        send_segment(_seg);
        _cansend = win - (_next_seqno - _ackno);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size = window_size;
    uint64_t ack = unwrap(ackno, _isn, _ackno);
    if (ack > _next_seqno) return false;
    if (_ackno < ack) {
        _ackno = ack;
        _rto = _initial_retransmission_timeout;
        _consecutive_retran_count = 0;
        _timer_count = 0;
    }
    else return false;
    while (!_segment_outstanding.empty()) {
        TCPHeader _header = _segment_outstanding.front().header();
        if (unwrap(_header.seqno, _isn, _ackno) + _segment_outstanding.front().length_in_sequence_space() <= _ackno) {
            _bytes_in_flight -= _segment_outstanding.front().length_in_sequence_space();
            _segment_outstanding.pop();
        }
        else break;
    }
    if (_segment_outstanding.empty()) {
        _timer_started = 0;
    }
    fill_window();
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if (_timer_started == 0) {
        return ;
    }
    _timer_count += ms_since_last_tick;
    if (_timer_count >= _rto) {
        TCPSegment _firstseg = _segment_outstanding.front();
        _segments_out.push(_firstseg);
        _consecutive_retran_count++;
        if (unwrap(_firstseg.header().seqno, _isn, _ackno) - _ackno + _firstseg.length_in_sequence_space() <= _window_size)
            _rto *= 2;        // double rto
        _timer_started = 1;
        _timer_count = 0; //reset timer
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return {_consecutive_retran_count}; }

void TCPSender::send_empty_segment() {
    TCPSegment _seg;
    _seg.header().seqno = next_seqno();
    _segments_out.push(_seg);
}

void TCPSender::send_segment(TCPSegment& seg) {
    _next_seqno += seg.length_in_sequence_space();
    _segments_out.push(seg);
    if (seg.length_in_sequence_space() != 0) {
        _segment_outstanding.push(seg);
    }
    _bytes_in_flight += seg.length_in_sequence_space();
    if (_timer_started == 0) {
        _timer_started = 1;
        _timer_count = 0;
    }
}
