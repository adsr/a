<?php

class BufferTest extends PHPUnit_Framework_TestCase {

    function testLoadFromText() {
        $text = "sample\ntext\n";
        $buffer = new Buffer();
        $buffer->loadFromText($text);
        $this->assertEquals($text, (string)$buffer);
        return $buffer;
    }

    /** @depends testLoadFromText */
    function testGetLineCount($buffer) {
        $this->assertEquals(3, $buffer->getLineCount());
    }

    /** @depends testLoadFromText */
    function testGetCharCount($buffer) {
        $this->assertEquals(12, $buffer->getCharCount());
    }

    /** @depends testLoadFromText */
    function testGetLineAndOffset($buffer) {
        $this->assertEquals(array(0, 0), $buffer->getLineAndOffset(0));
        $this->assertEquals(array(0, 1), $buffer->getLineAndOffset(1));
        $this->assertEquals(array(2, 0), $buffer->getLineAndOffset(12));
        $this->assertEquals(array(1, 4), $buffer->getLineAndOffset(11));
        $this->assertEquals(array(2, 0), $buffer->getLineAndOffset(99));
        $this->assertEquals(array(0, 0), $buffer->getLineAndOffset(-99));
    }

    /** @depends testLoadFromText */
    function testGetBufferOffset($buffer) {
        $this->assertEquals(0, $buffer->getBufferOffset(0, 0));
        $this->assertEquals(1, $buffer->getBufferOffset(0, 1));
        $this->assertEquals(12, $buffer->getBufferOffset(2, 0));
        $this->assertEquals(11, $buffer->getBufferOffset(1, 4));
        $this->assertEquals(12, $buffer->getBufferOffset(99, 99));
        $this->assertEquals(0, $buffer->getBufferOffset(-99, -99));
        $this->assertEquals(7, $buffer->getBufferOffset(1, 0));
    }

    /**
     * @dataProvider dataProviderSpliceString
     */
    function testSpliceString($description, $text_in, $splice_string_params, $splice_string_return, $text_out) {
        $buffer = new Buffer();
        $buffer->loadFromText($text_in);
        $actual_splice_string_return = call_user_func_array(array($buffer, 'spliceString'), $splice_string_params);
        $this->assertEquals(
            $text_out,
            (string)$buffer,
            "{$description}: buffer"
        );
        $this->assertEquals(
            $splice_string_return,
            $actual_splice_string_return,
            "{$description}: return payload"
        );
    }

    function dataProviderSpliceString() {
        return array(
            array(
                'simple prepend',
                "sample\ntext",
                array(0, 0, 'lots of ', 0),
                array(0, 8, 8),
                "lots of sample\ntext"
            ),
            array(
                'simple append 1',
                "sample\ntext",
                array(0, 11, '!', 0),
                array(1, 5, 12),
                "sample\ntext!"
            ),
            array(
                'simple append 2',
                "sample\ntext\n",
                array(0, 12, '!', 0),
                array(2, 1, 13),
                "sample\ntext\n!"
            ),
            array(
                'simple delete',
                "sample\ntext\n",
                array(0, 0, '', 6),
                array(0, 0, 0),
                "\ntext\n"
            ),
            array(
                'prepend with delete',
                "sample\ntext",
                array(0, 0, 'lots of sample', 6),
                array(0, 14, 14),
                "lots of sample\ntext"
            ),
            array(
                'delete with newline',
                "sample\ntext\n",
                array(0, 4, '', 4),
                array(0, 4, 4),
                "sampext\n"
            ),
            array(
                'complete replace',
                "this text will go\naway",
                array(0, 0, "vanish", 22),
                array(0, 6, 6),
                "vanish"
            ),
            array(
                'complete delete',
                "this text will go\naway",
                array(0, 0, '', 22),
                array(0, 0, 0),
                ''
            ),
            array(
                'insert with newline and delete',
                "unseeming text",
                array(0, 9, "ly\ngreat s", 2),
                array(1, 7, 19),
                "unseemingly\ngreat sext"
            ),
            array(
                'insert and delete with newline',
                "cran\nDELETE\nME!!!!berry",
                array(0, 4, "apple\nberry", 14),
                array(1, 5, 15),
                "cranapple\nberryberry"
            ),
            array(
                'insert newline',
                "unaffected\napplebanana",
                array(1, 5, "\n", 0),
                array(2, 0, 17),
                "unaffected\napple\nbanana"
            ),
            array(
                'delete at beginning of line',
                "abcd\n",
                array(1, -1, '', 1),
                array(0, 4, 4),
                "abcd"
            ),
            array(
                'delete on empty buffer',
                '',
                array(0, 0, '', 1),
                array(0, 0, 0),
                ''
            ),
            array(
                'insert char at eol',
                "line\nline2",
                array(0, 4, '1', 0),
                array(0, 5, 5),
                "line1\nline2"
            ),
        );
    }

}
