## wPerf: Generic Off-CPU Analysis to Identify Bottleneck Waiting Events

This repository is the implementation of wPerf.
There are several modules in this repo:
- /annotation - Users can use the code to annotate waiting events which do not go through the kernel.
- /analyzer - wPerf's analyzer to generate the wait-for graph.
- /module - wPerf's kernel module to facilitate recording waiting events in kernel low level.
- /record - wPerf's recorder.


wPerf is designed to identify bottlenecks caused by all kinds of waiting events.
To identify waiting events that limit the application’s throughput, wPerf uses cascaded re-distribution to compute
the local impact of a waiting event and uses wait-for graph to compute whether such impact can reach other threads.


Check our papers for more details: [wPerf: Generic Off-CPU Analysis to Identify Bottleneck Waiting Events](https://www.usenix.org/system/files/osdi18-zhou.pdf), OSDI 2018.

## Requirements:
- [Kernel](http://www.kernel.org/): version 4.4 and newer. You also need to enable the KProbe and CONFIG_SCHEDSTATS features.
  
  We tested wPerf on several kernel versions: 4.4, 4.7, 4.9, and 4.13 and it worked well. For the kernel version below 4.4, once the mentioned features are enabled, it should work.
- [Python 2](http://www.python.org/)

  You also need to install two python libraries: [PrettyTable](https://pypi.org/project/PrettyTable/) and [NetworkX](https://networkx.github.io/).
- perf. perf should support dwarf call-graph profiling.
- iostat: version >= 11.2.0, used to record disk IOPS.
- ifstat: version >= 1.1, used to record NIC bandwidth.
- gdb: connect thread name to thread id for c programs.
- jstack: connect thread name to thread id for java programs.
- [perf-map-agent](https://github.com/jvm-profiling-tools/perf-map-agent): For recording call stacks of java applications. Copy this directory into wPerf/
- wPerf uses /tmp/ to store some intermediate results. Make sure that you have this directory.

## Compile your application:
- Annotate if necessary: If there are any waiting events, such as RDMA operations and spinlocks, that do not go through kernel, you have to annotate by yourself.

For C programs, copy the code in wPerf/annotation/c/main.c into the main function and the code in wPerf/annotation/c/thread.c into the code where you want to annotate.

For Java programs, run make in wPerf/annotation/java/ and include the annotation library in your code.

Then for both kinds of programs, use uds_add(&address, event_type) to record the such waiting events.

- C program: compile your program with "-g" option.
- Java program: start JVM with the option "-XX:+PreserveFramePointer", which has been added since JDK8u60.

## Compile wPerf kernel module and recorder:
Run make in the repo's root directory.

## Test your application with wPerf:
After you start your application, you can run wPerf/recorder/record.sh and follow the instructions.

## Generate wait-for graph:
Once the test is finished, you can run wPerf/analyzer/analyzer.sh to generate the wait-for graph. 
Then use wPerf/analyzer/knot.sh to start the python program identifying the bottlenecks in your application. 
Use 'g' with the bottleneck component number to generate a csv file, which contains the bottleneck.

## Show time:
Use our [online graph explorer](https://osusyslab.github.io/wperf/) with your result file and check out the bottleneck in that knot.

As one can see from the screenshot, the left part shows the graph information and the right part shows the wait-for graph.
### Screenshot:
![Screenshot of wPerf](https://https://github.com/OSUSysLab/OSUSysLab.github.io/blob/master/wperf/wPerf_screenshot.png)

## Contact
If you have any questions, please contact Fang at zhou.1250@osu.edu, Yifan at gan.101@osu.edu, Sixiang at ma.1189@osu.edu, and Yang at wang.7564@osu.edu
