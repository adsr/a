<?php

interface IBufferListener {
    function onDirtyLines($start_line, $end_line, $line_count_decreased);
}
