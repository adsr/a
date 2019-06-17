package main

type EventType int

const (
    EVENT_TYPE_TONE EventType = iota
    EVENT_TYPE_PERC
    EVENT_TYPE_CTL
    EVENT_TYPE_RAW
    EVENT_TYPE_CODE
)

type MidiMsg struct {
    Status int
    Data1  int
    Data2  int
}

type Event struct {
    EventType    EventType
    PlayAtBeat   uint64
    DeviceName   string
    Channel      int
    Notes        []int
    Vol          int
    BeatLen      int
    Control      string
    CCNum        int
    CCVal        int
    Description  string
    Code         string
    MidiMsgs     []MidiMsg
    MidiSysex    []int
    midiSysexStr string
}

func (event *Event) Clone() *Event {
    copiedEvent := new(Event)
    *copiedEvent = *event
    copy(copiedEvent.Notes, event.Notes)
    copy(copiedEvent.MidiMsgs, event.MidiMsgs)
    copy(copiedEvent.MidiSysex, event.MidiSysex)
    return copiedEvent
}

func (event *Event) SetMidiSysex(sysex []int) {
    sysexBytes := make([]byte, 0, len(sysex))
    for _, num := range sysex {
        sysexBytes = append(sysexBytes, byte(num))
    }
    event.MidiSysex = sysex
    event.midiSysexStr = string(sysexBytes)
}

func (event *Event) MakeMidi() {
    switch event.EventType {
    case EVENT_TYPE_PERC:
        fallthrough
    case EVENT_TYPE_TONE:
        event.MidiMsgs = make([]MidiMsg, 0, len(event.Notes))
        for _, note := range event.Notes {
            event.MidiMsgs = append(event.MidiMsgs, MidiMsg{0x90 + event.Channel, note, event.Vol})
        }
    case EVENT_TYPE_CTL:
        event.MidiMsgs = make([]MidiMsg, 1)
        event.MidiMsgs[0].Status = 0xb0 + event.Channel
        event.MidiMsgs[0].Data1 = event.CCNum
        event.MidiMsgs[0].Data2 = event.CCVal
    }
}

func (event *Event) MakeNoteOff(track *Track) {
    for i := range event.MidiMsgs {
        event.MidiMsgs[i].Status = 0x80 + (0x0f & event.MidiMsgs[i].Status)
        event.MidiMsgs[i].Data2 = 0
    }
    event.PlayAtBeat += uint64(event.BeatLen) * track.BeatDivisor
}
