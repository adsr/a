package main

type MidiEventFilter struct {
    NoteMap    map[int]int
    ControlMap map[int]int
}

func NewMidiEventFilter() *MidiEventFilter {
    filter := new(MidiEventFilter)
    filter.NoteMap = make(map[int]int)
    filter.ControlMap = make(map[int]int)
    for i := 0; i < 128; i++ {
        filter.NoteMap[i] = i
        filter.ControlMap[i] = i
    }
    return filter
}
