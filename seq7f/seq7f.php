#!/usr/bin/env php
<?php

class Seq7f {
    private $in_counts;
    private $in_data;
    private $in_dev;
    private $in_port;
    private $list_midi;
    private $midi_queue;
    private $nalive;
    private $now;
    private $out_dev;
    private $out_port;
    private $seq_paths;
    private $seqs;
    private $shared_obj;
    private $show_help;
    private $t;

    const DUR_GRAN = 8;

    function showUsage() {
        echo "Usage: {$_SERVER['PHP_SELF']} <options> -- <seq> [<seq> ...]\n\n" .
            "Options:\n" .
            "  -h        Show this help\n" .
            "  -i <port> Listen on this MIDI port\n" .
            "  -o <port> Write to this MIDI port\n" .
            "  -L        List MIDI ports\n";
    }

    function parseOpts() {
        $opt = getopt('hi:o:L');
        $this->show_help = isset($opt['h']);
        $this->list_midi = isset($opt['L']);
        $this->in_port = isset($opt['i']) ? $opt['i'] : null;
        $this->out_port = isset($opt['o']) ? $opt['o'] : null;
        $this->seq_paths = [];
        $args = $_SERVER['argv'];
        array_shift($args);
        $seq_paths = [];
        array_walk($args, function($arg, $i) use (&$seq_paths) {
            if (is_readable($arg)) {
                $seq_paths[] = $arg;
            }
        });
        $this->seq_paths = $seq_paths;
    }

    function run() {
        $inited = false;
        try {
            $this->parseOpts();
            if ($this->show_help) {
                $this->showUsage();
                return;
            }
            if (!portmidi_initialize()) {
                throw new RuntimeException('Failed to init portmidi');
            }
            $inited = true;
            if ($this->list_midi) {
                $this->listMidi();
            } else {
                $this->loadSeqs();
                $this->openMidiPorts();
                $this->loop();
            }
        } catch (RuntimException $e) {
            throw $e;
        } finally {
            if ($inited) {
                $this->closeMidiPorts();
                portmidi_terminate();
            }
        }
    }

    function listMidi() {
        $infos = [];
        for ($i = 0; $i < portmidi_count_devices(); $i++) {
            $infos[] = portmidi_get_device_info($i);
        }
        $max_name_len = array_reduce($infos, function($rv, $info) {
            $slen = strlen($info['name']);
            return $slen > $rv ? $slen : $rv;
        }, 0);
        $fmt = "%3s %-{$max_name_len}s %-8s %-8s\n";
        printf($fmt, '', 'Name', 'Type', 'Opened?');
        foreach ($infos as $i => $info) {
            printf($fmt,
                $i,
                $info['name'],
                $info['input'] ? 'In' : 'Out',
                $info['opened'] ? 'Y' : 'N'
            );
        }
    }

    function openMidiPorts() {
        if ($this->in_port) {
            $this->in_dev = $this->openMidiPort($this->in_port, $is_input = true);
        }
        if ($this->out_port) {
            $this->out_dev = $this->openMidiPort($this->out_port, $is_input = false);
        }
    }

    function closeMidiPorts() {
        if ($this->in_dev) {
            portmidi_close($this->in_dev);
        }
        if ($this->out_dev) {
            portmidi_close($this->out_dev);
        }
    }

    function openMidiPort($port, $is_input) {
        if (!is_numeric($port)) {
            $found = false;
            for ($i = 0; $i < portmidi_count_devices(); $i++) {
                $info = portmidi_get_device_info($i);
                if ($is_input == $info['input']
                    && stripos($info['name'], $port) !== false
                ) {
                    $port = $i;
                    $found = true;
                    break;
                }
            }
            if (!$found) {
                throw new RuntimeException('Port not found');
            }
        }
        $port = (int)$port;
        if ($port < 0 || $port >= portmidi_count_devices()) {
            throw new RuntimeException('Port num out of range');
        }
        $dev = $is_input
            ? portmidi_open_input($port)
            : portmidi_open_output($port);
        if (!$dev) {
            throw new RuntimeException('Failed to open port');
        }
        return $dev;
    }

    function loadSeqs() {
        $this->seqs = [];
        $seq7f = $this;
        array_walk($this->seq_paths, function($seq_path, $i) use ($seq7f) {
            require $seq_path;
        });
    }

    function registerSeq($seq) {
        if ($seq instanceof Seq) {
            $this->seqs[] = $seq;
        } else {
            throw new RuntimeException('Expected instanceof Seq');
        }
    }

    function loop() {
        $this->t = 0;
        $this->shared_obj = new stdClass();
        $this->in_data = [];
        $this->in_counts = [];
        $this->nalive = count($this->seqs);
        $this->midi_queue = [];
        while (true) {
            $this->now = microtime(true) * 1000;
            $this->readEvents();
            $delay = $this->processSeqs();
            if ($this->nalive < 1) {
                break;
            }
            $this->sendMidi();
            $delay -= (microtime(true) * 1000) - $this->now;
            if ($delay > 0) {
                usleep($delay * 1000);
            }
            $this->t += 1;
        }
    }

    function readEvents() {
        if (!$this->in_dev) {
            return;
        }
        do {
            $ev = portmidi_read($this->in_dev);
            if ($ev && $ev['status'] & 0xb0) {
                $this->in_data[$ev['data1']] = $ev['data2'];
                $this->in_counts[$ev['data1']] += ($ev['data2'] > 0 ? 1 : 0);
            }
        } while ($ev !== null);
    }

    function processSeqs() {
        $common_delay = $this->getGreatCommonDelay();
        $nalive = 0;
        foreach ($this->seqs as $seq) {
            if ($this->get($seq, 'len') <= $this->t) {
                continue;
            }
            $nalive += 1;
            $delay = $this->get($seq, 'delay');
            if ((int)$seq->delay_rem <= 0) {
                $seq->delay_rem = $delay;
            }
            $seq->delay_rem -= $common_delay;
            if ($seq->delay_rem > 0) {
                continue;
            } else if (mt_rand() / mt_getrandmax() > $this->get($seq, 'prob')) {
                continue;
            }

            $scale = $this->get($seq, 'scale');
            $nscale = count($scale) - 1;
            $octave = end($scale);
            $key = $this->get($seq, 'key');
            $indexes = $this->get($seq, 'indexes');

            $vals = [];
            if ($nscale > 0) {
                foreach ($indexes as $index) {
                    $vals[] = min(127, max(0,
                        $key
                        + (intdiv($index, $nscale) * $octave)
                        + $scale[$index % $nscale]
                    ));
                }
            }

            $this->queueMidi(
                $vals,
                min(127, max(0, (int)$this->get($seq, 'vel'))),
                max(1, (float)$this->get($seq, 'gate') * $delay),
                min(15, max(0, $seq->channel)),
                $seq->use_note_off
            );

            $seq->t += $this->get($seq, 'step');
        }
        $this->nalive = $nalive;
        return $common_delay;
    }

    function get($seq, $what) {
        return $seq->get($what, $seq->t, $this->t, $this->in_data, $this->in_counts, $this->shared_obj);
    }

    function queueMidi($vals, $vel, $dur_ms, $channel, $use_note_off) {
        foreach ($vals as $val) {
            $this->midi_queue[] = [0, 0x90 + $channel, $val, $vel];
            if ($use_note_off) {
                $this->midi_queue[] = [$this->now + $dur_ms, 0x80 + $channel, $val, 0];
            }
        }
    }

    function sendMidi() {
        foreach ($this->midi_queue as $k => $msg) {
            if ($this->now >= $msg[0]) {
                if ($this->out_dev) {
                    portmidi_write_short($this->out_dev, $msg[1], $msg[2], $msg[3]);
                }
                printf("%2x %2x %2x\n", $msg[1], $msg[2], $msg[3]);
                //printf("%3d %3d %3d\n", $msg[1], $msg[2], $msg[3]);
                unset($this->midi_queue[$k]);
            }
        }
    }

    function getGreatCommonDelay() {
        $delays = [];
        foreach ($this->seqs as $seq) {
            $delays[] = $this->get($seq, 'delay');
        }
        return max(self::DUR_GRAN, intdiv($this->gcf($delays), self::DUR_GRAN));
    }

    function gcf($values) {
        $gcf = 1;
        $x = reset($values);
        $y = next($values);
        if (count($values) < 2) {
            return $x;
        }
        for ($i = 1; $i < count($values); $i++) {
            $a = max($x, $y);
            $b = min($x, $y);
            $c = 1;
            do {
                if ($b == 0) {
                    return 1;
                }
                $c = $a % $b;
                $gcf = $b;
                $a = $b;
                $b = $c;
            } while ($c !== 0);
            $x = $gcf;
            $y = next($values);
        }
        return $gcf;
    }
}

abstract class Seq {
    public $indexes = [ 36 ];
    public $scale = [2, 1, 2, 2, 1, 2, 2];
    public $key = 0;
    public $delay = 250;
    public $step = 1;
    public $chord = [ 0 ];
    public $vel = 100;
    public $gate = 1;
    public $prob = 1.0;
    public $len = 720;
    public $delay_rem = 0;
    public $index = 0;
    public $use_note_off = true;
    public $channel = 0;
    public $t = 0;
    function indexesFn($t, $k, $i, $c, $o) {}
    function scaleFn($t, $k, $i, $c, $o) {}
    function keyFn($t, $k, $i, $c, $o) {}
    function delayFn($t, $k, $i, $c, $o) {}
    function stepFn($t, $k, $i, $c, $o) {}
    function chordFn($t, $k, $i, $c, $o) {}
    function velFn($t, $k, $i, $c, $o) {}
    function gateFn($t, $k, $i, $c, $o) {}
    function probFn($t, $k, $i, $c, $o) {}
    function lenFn($t, $k, $i, $c, $o) {}
    function get() {
        $args = func_get_args();
        $prop = array_shift($args);
        if (isset($this->$prop)) {
            return $this->$prop;
        }
        $fn = "{$prop}Fn";
        return call_user_func_array([$this, $fn], $args);
    }
}

(new Seq7f())->run();
