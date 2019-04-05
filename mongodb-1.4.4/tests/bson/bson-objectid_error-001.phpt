--TEST--
MongoDB\BSON\ObjectId #001 error
--SKIPIF--
<?php if (defined("HHVM_VERSION_ID")) exit("skip HHVM handles parameter parsing differently"); ?>
--FILE--
<?php

require_once __DIR__ . '/../utils/tools.php';

throws(function() {
    new MongoDB\BSON\ObjectId(new stdclass);
}, "MongoDB\\Driver\\Exception\\InvalidArgumentException");

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
OK: Got MongoDB\Driver\Exception\InvalidArgumentException
===DONE===

