
#!/bin/bash

echo "clean redis"
make clean

echo "build redis"
make && make install


