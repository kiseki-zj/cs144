#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return {_sender.stream_in().remaining_capacity()}; }

size_t TCPConnection::bytes_in_flight() const { return {_sender.bytes_in_flight()}; }

size_t TCPConnection::unassembled_bytes() const { return {_receiver.unassembled_bytes()}; }

size_t TCPConnection::time_since_last_segment_received() const { return {_time_since_last_recv}; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    bool send_empty = false;
    _time_since_last_recv = 0;
    if (!active()) return ;
    if (seg.header().rst == true) {
        unclean_shutdown();
        return ;
    }
    if (seg.header().ack) {
        if (!_sender.syn_sent()) {
            return;
        }
        else
            _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    _receiver.segment_received(seg);
    if (seg.header().syn && !_sender.syn_sent()) {
        connect();
        return ;
    }
    
    if (seg.header().fin) {
        //if (!_sender.fin_sent())
        //    _sender.fill_window();
        if (!_sender.fin_sent()) {
            _sender.fill_window();
        }
        send_empty = true;
    }
    else if (seg.length_in_sequence_space()) {
        send_empty = true;
    }
    if (send_empty) {
        if (_sender.segments_out().empty())
            _sender.send_empty_segment();
    }
    push_segments();
    check_shutdown();
}

bool TCPConnection::active() const { return {_active}; }

size_t TCPConnection::write(const string &data) {
    size_t sz = _sender.stream_in().write(data);
    return {sz};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender._sendsyn = false;
    if (!active()) return;
    _time_since_last_recv += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        _send_rst = true;
        _sender.send_empty_segment();
    }
    push_segments();
    check_shutdown();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    push_segments();
    check_shutdown();
}

void TCPConnection::connect() {
    _sender._sendsyn = true;
    push_segments();
}


TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            //destruct when active
            //send rst
            _send_rst = true;
            _sender.send_empty_segment();
            push_segments();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::unclean_shutdown() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
}

void TCPConnection::push_segments() {
    _sender.fill_window();
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        if (_send_rst) {
            seg.header().rst = true;
            unclean_shutdown();
            _segments_out.push(seg);
            _sender.segments_out().pop();
            return;
        }
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        seg.header().win = _receiver.window_size();
        _segments_out.push(seg);
    }
}
void TCPConnection::check_shutdown() {
    //passive?
    if (_receiver.stream_out().input_ended() && (!_sender.stream_in().eof()))
        _linger_after_streams_finish = false;
    //active close, prereq#1~#4
    if (_linger_after_streams_finish && _receiver.stream_out().eof() && _sender.stream_in().eof() && _sender.fin_sent() && bytes_in_flight()==0 && _time_since_last_recv >= 10 * _cfg.rt_timeout)
        _active = false;
    //passive close, prereq#1~#3
    if (!_linger_after_streams_finish && _receiver.stream_out().eof() && _sender.stream_in().eof() && _sender.fin_sent() && bytes_in_flight() == 0)
        _active = false;
}