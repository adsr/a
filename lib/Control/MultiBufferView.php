<?php

/**
 * Control that represent 1 or more BufferViews
 */
class Control_MultiBufferView extends Control {

    /** @var array [<Control_MultiBufferView_Node>, ...] */
    private $root_nodes;

    /** @var Control_MultiBufferView_Node */
    private $active_node;

    /** Load new node */
    public function __construct() {
        $node = new Control_MultiBufferView_Node();
        //$node->split(1, 80);
        $this->root_nodes = array($node);
        $this->active_node = $this->root_nodes[0];
    }

    /** Resize according to params */
    public function resize($rows, $cols, $line, $offset) {
        parent::resize($rows, $cols, $line, $offset);
        foreach ($this->root_nodes as $root_node) {
            $root_node->resize($rows, $cols, $line, $offset);
        }
    }

    /** Refresh display */
    public function refresh() {
        $this->active_node->refresh();
    }

    /** @return BufferView */
    public function getActiveBufferView() {
        return $this->active_node->getBufferView();
    }

    /** @return array [<line>, <offset>, <visibility flag>] */
    public function getCursorState() {
        return $this->getActiveBufferView()->getCursorState();
    }

    /** @param Registry $registry */
    public function setRegistry(Registry $registry) {
        parent::setRegistry($registry);
        foreach ($this->root_nodes as $root_node) {
            $root_node->setRegistry($registry);
        }
    }

}
