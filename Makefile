hello:
	@ipcrm -a
	@g++ producer.cpp -o producer
	@g++ consumer.cpp -o consumer
