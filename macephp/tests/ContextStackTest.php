<?php

class ContextStackTest extends PHPUnit_Framework_TestCase {

    function testRootContext() {
        $stack = new ContextStack();
        $this->assertEquals(ContextStack::ROOT_CONTEXT, $stack->getContextName());
        return $stack;
    }

    /** @depends testRootContext */
    function testCurrentCommand($stack) {
        $command = $this->getMockBuilder('Command')
            ->disableOriginalConstructor()
            ->getMock();
        $stack->setCurrentCommand($command);
        $this->assertEquals($command, $stack->getCurrentCommand());
    }

    /** @depends testRootContext */
    function testActiveControl($stack) {
        $control = $this->getMock('Control');
        $stack->setActiveControl($control);
        $this->assertEquals($control, $stack->getActiveControl());
    }

    /** @depends testRootContext */
    function testPushAndPop($stack) {
        $this->assertEquals(0, $stack->getDepth());
        $stack->push('test');
        $this->assertEquals('test', $stack->getContextName());
        $this->assertEquals(1, $stack->getDepth());
        $stack->push('test2');
        $this->assertEquals('test.test2', $stack->getContextName());
        $this->assertEquals(2, $stack->getDepth());
        $this->assertEquals('test2', $stack->pop());
        $this->assertEquals('test', $stack->getContextName());
        $this->assertEquals(1, $stack->getDepth());
        $this->assertEquals('test', $stack->pop());
        $this->assertEquals(ContextStack::ROOT_CONTEXT, $stack->getContextName());
        $this->assertEquals(0, $stack->getDepth());
    }

    /**
     * @depends testRootContext
     * @expectedException RuntimeException
     */
    function testPopInvalid($stack) {
        $stack->pop();
    }

    /**
     * @depends testRootContext
     * @expectedException InvalidArgumentException
     */
    function testPushInvalid1($stack) {
        $stack->push('invalid.name');
    }

    /**
     * @depends testRootContext
     * @expectedException InvalidArgumentException
     */
    function testPushInvalid2($stack) {
        $stack->push('');
    }

    /**
     * @depends testRootContext
     * @expectedException InvalidArgumentException
     */
    function testPushInvalid3($stack) {
        $stack->push(null);
    }

    /**
     * @depends testRootContext
     * @expectedException InvalidArgumentException
     */
    function testPushInvalid4($stack) {
        $stack->push("invalid\n");
    }

    /**
     * @depends testRootContext
     * @expectedException InvalidArgumentException
     */
    function testPushInvalid5($stack) {
        $stack->push("\xff");
    }

    /**
     * @depends testRootContext
     * @expectedException InvalidArgumentException
     */
    function testPushInvalid6($stack) {
        $stack->push('x/y');
    }

}
