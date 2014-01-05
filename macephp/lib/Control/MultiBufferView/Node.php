<?php

/**
 * Represents a node in the MultiBufferView tree.
 * This can be a wrapper for 1 BufferView or a container
 * of two BufferViews after a split.
 */
class Control_MultiBufferView_Node extends Control {

    /** @const int */
    const SPLIT_TYPE_VERTICAL = 1;

    /** @const int */
    const SPLIT_TYPE_HORIZONTAL = 2;

    /** @var Control_MultiBufferView_Node */
    private $parent;

    /** @var bool */
    private $is_container;

    /** @var int self::SPLIT_TYPE_* value */
    private $split_type;

    /** @var int */
    private $split_position;

    /** @var Control_MultiBufferView_Node */
    private $split_active_node;

    /** @var Control_MultiBufferView_Node */
    private $split_orig_node;

    /** @var Control_MultiBufferView_Node */
    private $split_new_node;

    /** @var Control_BufferView */
    private $buffer_view;

    /** Load a new BufferView */
    public function __construct() {
        // TODO load file?
        $this->buffer_view = new Control_BufferView();
    }

    /** @param Registry $registry */
    public function setRegistry(Registry $registry) {
        parent::setRegistry($registry);
        if ($this->is_container) {
            $this->split_orig_node->buffer_view->setRegistry($registry);
            $this->split_new_node->buffer_view->setRegistry($registry);
        }
        else {
            $this->buffer_view->setRegistry($registry);
        }
    }

    /** @return Registry */
    public function getBufferView() {
        if ($this->is_container) {
            return $this->split_active_node->buffer_view;
        }
        else {
            return $this->buffer_view;
        }
    }

    /** Split node into two nodes */
    public function split($type, $position) {
        // TODO throw exception if is_container==TRUE
        $orig_node = clone $this;
        $new_node = clone $this;
        $this->is_container = true;
        $this->split_type = $type;
        $this->split_position = $position;
        $this->split_orig_node = $orig_node;
        $this->split_active_node = $orig_node;
        $this->split_new_node = $new_node;
        $this->split_new_node->buffer_view->setFocus(false);
        $orig_node->parent = $this;
        $new_node->parent = $this;
        if ($type === self::SPLIT_TYPE_HORIZONTAL) {
            $prop_dimension = 'rows';
            $prop_position = 'line';
        }
        else {
            $prop_dimension = 'cols';
            $prop_position = 'offset';
        }

        $orig_node->$prop_dimension = $position;
        $new_node->$prop_position += $position;
        $new_node->$prop_dimension -= $position;
    }

    /** Unsplit */
    public function unsplit() {
        // Only applies to leaf nodes with parent nodes
        // Replace parent with sibling
    }

    /** Resize display according to params */
    public function resize($rows, $cols, $line, $offset) {

        $rows_factor = $this->rows ? (float)($rows / $this->rows) : 1;
        $cols_factor = $this->cols ? (float)($cols / $this->cols) : 1;
        $this->rows = $rows;
        $this->cols = $cols;
        $this->line = $line;
        $this->offset = $offset;

        if ($this->is_container) {
            if ($this->split_type === self::SPLIT_TYPE_HORIZONTAL) {
                $position = floor($rows_factor * $this->split_position);
                $this->split_orig_node->resize(
                    $position,
                    $this->cols,
                    $this->line,
                    $this->offset
                );
                $this->split_new_node->resize(
                    $this->rows - $position,
                    $this->cols,
                    $this->line + $position,
                    $this->offset
                );
            }
            else {
                $position = floor($cols_factor * $this->split_position);
                $this->split_orig_node->resize(
                    $this->rows,
                    $position,
                    $this->line,
                    $this->offset
                );
                $this->split_new_node->resize(
                    $this->rows,
                    $this->cols - $position,
                    $this->line,
                    $this->offset + $position
                );
            }
        }
        else {
            $this->buffer_view->resize($this->rows, $this->cols, $this->line, $this->offset);
        }

    }

    /** Refresh display */
    public function refresh() {
        if ($this->is_container) {
            $this->split_orig_node->refresh();
            $this->split_new_node->refresh();
        }
        else {
            $this->buffer_view->refresh();
        }
    }

    /** Clone */
    public function __clone() {
        $this->buffer_view = clone $this->buffer_view;
    }

}
