macephp
=======

NOTE: macephp is now abandoned in favor of [mace](https://github.com/adsr/mace), the C port.

macephp is a console-based text editor written in PHP.

**Requirements**
* PHP 5.3
* PHP ncurses extension (http://www.php.net/manual/en/ncurses.installation.php)

**Install and run**

1. Install PHP ncurses extension:
  <pre>
  $ sudo pecl install ncurses
  </pre>

  Follow the instructions from pecl. Make sure to add "extension=ncurses.so" to php.ini.

2. Clone repo and run:
  <pre>
  $ git clone git://github.com/adsr/macephp.git
  $ cd macephp
  $ ./mace
  </pre>
