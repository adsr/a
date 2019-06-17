<?php

class Derp1 extends Seq {
    public $indexes = null;
    public function indexesFn($t, $k, $i, $c, $o) {
        $degrees = [0,3,4, 7,10,11,  14,17,18];
        $mod = 7 + (isset($i[4]) ? $i[4] % 32 : 0);
        $tt = $t * $mod;
        $n = $degrees[$tt % count($degrees)];
        if ($o->lastn == $n) {
            return [];
        }
        $o->lastn = $n;
        if ($t % 16 == 0) {
            return [$n, $n+3, $n+4];
        }
        return [ $n ];
    }

    public $scale = null;
    function scaleFn($t, $k, $i, $c, $o) {
        if ($t % 80 < 40) {
            return [0, 1, 3, 5, 7, 8, 10, 12];
        }
        return [0, 2, 3, 5, 7, 8, 10, 12 ];
    }

    public $key = null;
    function keyFn($t, $k, $i, $c, $o) {
        if ($t % 80 < 40) {
            return 40;
        }
        return 42;
    }

    public $gate = null;
    function gateFn($t, $k, $i, $c, $o) {
        if ($t % 16 == 0) {
            return 16;
        }
        return isset($i[3]) ? ($i[3]/127.0) : 0.5;
    }

    public $delay = null; //130;
    function delayFn($t, $k, $i, $c, $o) {
        return isset($i[2]) ? min(260, max(30, $i[2]*2)) : 130;
    }

    public $len = 99999;

    public $step = null; //1;
    function stepFn($t, $k, $i, $c, $o) {
        return max(1, $i[5]);
    }
}

$seq7f->registerSeq(new Derp1());
