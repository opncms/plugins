DIRS = user
MAKE = make

all:
	set -e; for d in $(DIRS); do $(MAKE) -C $$d all ; done

clean:
	set -e; for d in $(DIRS); do $(MAKE) -C $$d clean ; done

