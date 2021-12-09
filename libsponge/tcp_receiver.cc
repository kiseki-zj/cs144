#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader _header = seg.header();
    Buffer _payload = seg.payload();
    
    WrappingInt32 _seqno(_header.seqno);
    uint64_t _index;
    bool eof = false;
    if (_header.syn) {
        _isn = WrappingInt32(_header.seqno);
        _state = SYN_RECV;
        _index = 0;
    }
    else _index = unwrap(_seqno, _isn, _reassembler.firstunassembled()) - 1;
    if (_header.fin) {
        eof = true;
    }

    _reassembler.push_substring(_payload.copy(), _index, eof);
    if (_reassembler.stream_out().input_ended())
        _state = FIN_RECV;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (_state == LISTEN) return {};
        else if (_state == FIN_RECV)
            return {wrap(_reassembler.firstunassembled()+2, _isn)};
    return {wrap(_reassembler.firstunassembled()+1, _isn)};
}

size_t TCPReceiver::window_size() const { return {_reassembler.stream_out().remaining_capacity()}; }
