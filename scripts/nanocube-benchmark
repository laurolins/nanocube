#!/usr/bin/env python
import argparse
import sys
import time
import traceback
import os
import urllib

if __name__ == "__main__":

    parser = argparse.ArgumentParser();
    parser.add_argument('-s', 
                        "--server", 
                        dest="server", 
                        type=str, 
                        required=True, 
                        help='Nanocube Server URL')
    parser.add_argument('-o', 
                        "--output", 
                        dest="output", 
                        type=str,
                        default="",
                        required=False, 
                        help='Output .psv file (same as the nc timing service, but includes downloading time too')
    parser.add_argument('-f', 
                        "--frequency", 
                        dest="frequency", 
                        type=int,
                        default=1000,
                        required=False, 
                        help='Frequency in which to give feedback')
    parser.add_argument('-m', 
                        "--maximum", 
                        dest="max", 
                        type=int,
                        default=0,
                        required=False, 
                        help='Run test with only the initial input queries. If 0 (default) all queries are going to be sent')
    parser.add_argument('-d', 
                        "--data", 
                        dest="data", 
                        type=str, 
                        default="",
                        required=False, 
                        help='Data File with a query per line (if not present use stdin)')

    args = parser.parse_args()
    tmp_filename = "/tmp/__x__"

    istream = sys.stdin if not len(args.data) else open(args.data,"r")
    ostream = sys.stdout if not len(args.output) else open(args.output,"w")

    ostream.write("finish_time|latency_ns|output_bytes|input_bytes|query_string\n")

    server = args.server
    try:
        server += '' if server[-1] == '/' else '/'
    except:
        sys.stderr.write("Invalid server name\n")
        sys.exit(0)

    queries_tried        = 0
    queries_completed    = 0
    total_time           = 0
    total_bytes_sent     = 0
    total_bytes_received = 0

    start_time = time.time()

    line_no = 0
    for line in istream:

        line_no += 1

        line = line.replace("'"," ").strip()
        url = server + line
        
        queries_tried += 1
        try:
            t0 = time.time()
            urllib.urlretrieve(url, filename=tmp_filename)
            t1 = time.time()
            elapsed_time = int((t1 - t0) * 1e9) # nanoseconds
            file_size = os.path.getsize(tmp_filename)
            
            final_datetime = time.strftime("%Y-%m-%d_%H:%M:%S",time.localtime(t1))

            total_time           += t1 - t0 # in seconds
            total_bytes_sent     += len(line) + 2
            total_bytes_received += file_size 
            
            queries_completed += 1

            ostream.write("%s|%d|%d|%d|'%s'\n" % (final_datetime,
                                                  elapsed_time,
                                                  file_size,
                                                  len(line) + 2, # single quotes
                                                  line))
        except:
            # traceback.print_exc()
            sys.stderr.write("error processing line %d\n" % line_no)
            
        if line_no % args.frequency == 0:
            sys.stderr.write("...processed %10d lines in %10.3f s.\n" % (line_no, time.time() - start_time))

        if args.max != 0 and line_no == args.max:
            break


    sys.stderr.write("...queries_tried:         %10d\n" % queries_tried)
    sys.stderr.write("...queries_completed:     %10d\n" % queries_completed)
    sys.stderr.write("...total_data_sent:           %10.3f MB\n" % (total_bytes_sent/1024.0/1024.0))
    sys.stderr.write("...total_data_received        %10.3f MB\n" % (total_bytes_received/1024.0/1024.0))
    sys.stderr.write("...total_time:                %10.3f s.\n" % total_time)
