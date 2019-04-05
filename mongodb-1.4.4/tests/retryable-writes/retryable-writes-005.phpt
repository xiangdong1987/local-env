--TEST--
Retryable writes: non-write command methods do not include transaction IDs
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"; ?>
<?php NEEDS_CRYPTO(); ?>
<?php NEEDS('REPLICASET'); CLEANUP(REPLICASET); ?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

class TransactionIdObserver implements MongoDB\Driver\Monitoring\CommandSubscriber
{
    public function commandStarted(MongoDB\Driver\Monitoring\CommandStartedEvent $event)
    {
        $command = $event->getCommand();
        $hasTransactionId = isset($command->lsid) && isset($command->txnNumber);

        printf("%s command includes transaction ID: %s\n", $event->getCommandName(), $hasTransactionId ? 'yes' : 'no');
    }

    public function commandSucceeded(MongoDB\Driver\Monitoring\CommandSucceededEvent $event)
    {
    }

    public function commandFailed(MongoDB\Driver\Monitoring\CommandFailedEvent $event)
    {
    }
}

$observer = new TransactionIdObserver;
MongoDB\Driver\Monitoring\addSubscriber($observer);

$manager = new MongoDB\Driver\Manager(REPLICASET, ['retryWrites' => true]);
$command = new MongoDB\Driver\Command([
    'findAndModify' => COLLECTION_NAME,
    'query' => ['x' => 1],
    'update' => ['$inc' => ['x' => 1]],
]);

echo "Testing Manager::executeCommand()\n";
$manager->executeCommand(DATABASE_NAME, $command);

echo "\nTesting Manager::executeReadCommand()\n";
$manager->executeReadCommand(DATABASE_NAME, $command);

echo "\nTesting Manager::executeReadWriteCommand()\n";
$manager->executeReadWriteCommand(DATABASE_NAME, $command);

echo "\nTesting Manager::executeWriteCommand()\n";
$manager->executeWriteCommand(DATABASE_NAME, $command);

MongoDB\Driver\Monitoring\removeSubscriber($observer);

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
Testing Manager::executeCommand()
findAndModify command includes transaction ID: no

Testing Manager::executeReadCommand()
findAndModify command includes transaction ID: no

Testing Manager::executeReadWriteCommand()
findAndModify command includes transaction ID: yes

Testing Manager::executeWriteCommand()
findAndModify command includes transaction ID: yes
===DONE===