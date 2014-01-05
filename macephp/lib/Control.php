<?php

/**
 * All on-screen controls extend this class
 */
abstract class Control {

    /** @var int */
    protected $rows;

    /** @var int */
    protected $cols;

    /** @var int */
    protected $line;

    /** @var int */
    protected $offset;

    /** @var Registry */
    protected $registry;

    /** Resize */
    public function resize($rows, $cols, $line, $offset) {
        $this->rows = $rows;
        $this->cols = $cols;
        $this->line = $line;
        $this->offset = $offset;
    }

    /** Refresh */
    abstract public function refresh();

    /** @return array [<line>, <offset>, <visibility flag>] */
    public function getCursorState() {
        return array(0, 0, 0);
    }

    /** @param Registry $registry */
    public function setRegistry(Registry $registry) {
        $this->registry = $registry;
    }

}
