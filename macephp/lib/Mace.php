<?php

/**
 * Main controller for editor
 */
class Mace {

    /** @var Control_TitleBar */
    private $title_bar;

    /** @var Control_MultiBufferView */
    private $multi_buffer_view;

    /** @var Control_StatusBar */
    private $status_bar;

    /** @var Control_PromptBar */
    private $prompt_bar;

    /** @var array array of Controls */
    private $controls = array();

    /** @var ContextStack */
    private $context_stack;

    /** @var CommandParser */
    private $command_parser;

    /** @var string path of rc file */
    private $rc_path;

    /** Instantiate controls */
    public function __construct() {
        error_reporting(E_ALL | E_STRICT);
        ini_set('log_errors', 1);
        ini_set('display_errors', 0);
    }

    /** Set rc path */
    public function setRcPath($rc_path) {
        $this->rc_path = $rc_path;
    }

    /** Main entry point */
    public function run() {
        $this->context_stack = new ContextStack();
        $this->command_parser = new CommandParser($this->context_stack);
        $this->processRcCommands();
        $this->initControls();
        $this->mainLoop();
    }

    /** Main loop */
    private function mainLoop() {

        $stdscr = Ncurses_Stdscr::getInstance();
        $input_parser = new InputParser(array($this, 'getInput'));

        // Program loop
        do {

            // Refresh all controls
            foreach ($this->controls as $control) {
                $control->refresh();
            }

            // Set cursor
            list($cursor_y, $cursor_x, $cursor_vis) = $this->context_stack->getActiveControl()->getCursorState();
            $stdscr->curs_set($cursor_vis);
            if ($cursor_vis) {
                $stdscr->wmove($cursor_y, $cursor_x);
            }

            // Send updates to client
            $stdscr->doupdate();

            // Read and execute next command
            $keychord = null;
            try {

                // Get command from input
                $keychord = $input_parser->getKeyChord(); // TODO unrecognized input
                if ($keychord === null) {
                    Config::set('status.text', 'Unrecognized input');
                    continue;
                }
                else if ($keychord == 'resize') {
                    // Special handling for resize keychord
                    $this->resize();
                    $stdscr->end();
                    $stdscr->refresh();
                }
                else {
                    Config::set('status.text', "last input was: $keychord");
                }

                // Get command from keychord
                $command = $this->command_parser->getCommandFromKeychord($keychord);

            }
            catch (CommandParser_Exception $e) {
                $this->setStatus($e->getMessage());
                $command_result = null;
                continue;
            }

            // Execute the command
            $this->context_stack->setCurrentCommand($command);
            $command_result = null;
            try {
                $command_result = $this->executeCommand($command);
            }
            catch (Exception $e) {
                Logger::error((string)$e, __CLASS__);
                $this->setStatus($e->getMessage());
            }

        } while ($command_result !== Command::$RESULT_HALT && $keychord !== 'q'); // TODO do not quit on 'q'

        $stdscr->clear();
        $stdscr->refresh();

    }

    /** @param string $str set status text */
    private function setStatus($str) {
        $this->status_bar->setStatus(stripslashes($str));
    }

    /** @param Command $command */
    private function executeCommand(Command $command) {
        $command_result = $command->execute();
        $this->context_stack->getRegistry()->setVar(
            Command::LAST_RESULT_VAR_NAME,
            $command_result,
            Registry::SCOPE_GLOBAL
        );
        return $command_result;
    }

    /** @return int */
    public function getInput() {
        return Ncurses_Stdscr::getInstance()->getch();
    }

    /** @param Control $control */
    private function addControl(Control $control) {
        $this->controls[] = $control;
        return $control;
    }

    /** Resize all controls */
    private function resize() {

        // TODO make control layout configurable / data driven
        list($height, $width) = Ncurses_Stdscr::getInstance()->getDimensions();
        Logger::debug("resizing to $width $height", __CLASS__);

        $this->title_bar->resize(1, $width, 0, 0);
        $this->status_bar->resize(1, $width, $height - 2, 0);
        $this->prompt_bar->resize(1, $width, $height - 1, 0);
        $this->multi_buffer_view->resize($height - 3, $width, 1, 0);

    }

    /** Execute rc commands */
    private function processRcCommands() {
        $rc_commands = array();
        if ($this->rc_path) {
            $rc_commands = array_filter(explode("\n", file_get_contents($this->rc_path)));
        }
        foreach ($rc_commands as $rc_command) {
            $command = $this->command_parser->getCommandFromStr($rc_command);
            try {
                $command->execute();
            } catch (Exception $e) {
                Logger::error(
                    "Startup command error",
                    __CLASS__,
                    $rc_command,
                    $e
                );
            }
        }
    }

    /** Set up all Controls */
    private function initControls() {

        $stdscr = Ncurses_Stdscr::getInstance();
        $stdscr->refresh();

        $this->title_bar = $this->addControl(new Control_TitleBar());
        $this->multi_buffer_view = $this->addControl(new Control_MultiBufferView());
        $this->status_bar = $this->addControl(new Control_StatusBar());
        $this->prompt_bar = $this->addControl(new Control_PromptBar());
        foreach ($this->controls as $control) {
            $control->setRegistry($this->context_stack->getRegistry());
        }

        $this->context_stack->setActiveControl($this->multi_buffer_view);

        $this->resize();

    }

}
