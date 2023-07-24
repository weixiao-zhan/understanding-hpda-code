### run netio example
0. start 2 shell
1. in one shell: `$ ./from_net 9000`
2. in the other shell: `$ ./to_net 9000`

### run full 3.4 example

let N := the number of worker node desired to run.
say using 2 workers.

0. `$ cd bin`
1. start N+2 shell
2. in one shell `$ ./aggregator 2`
2. in another shell `$ ./worker 0`
3. in another shell `$ ./worker 1`
4. in the last shell `$ ./splitter 2`