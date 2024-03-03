#!/usr/bin/env python3

import subprocess
import sys
import json
import tempfile

def log(msg):
    if '--grader' not in sys.argv:
        print(msg)


def silent_shell(cmd):
    sp = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        shell=True
    )
    if sp.returncode != 0:
        print(f'fatal: the command `${cmd}` failed')
        print(str(sp.stdout, 'utf-8'))
        print(str(sp.stderr, 'utf-8'))
        sys.exit(1)

def get_sec(time_str):
    parts = time_str.split(':')
    if len(parts) == 3:
        return int(parts[0]) * 3600 + int(parts[1]) * 60 + float(parts[0])
    else:
        return int(parts[0]) * 60 + float(parts[1])

def get_time_field(output, fieldname) -> int:
    try:
        fieldstart = output.index(fieldname) + len(fieldname)
        fieldend = output.index('\\n', fieldstart)
        # parse the number in the field (might be a percentage)
        if "wall" in fieldname:
            return get_sec(output[fieldstart:fieldend])
        else:
            return float(output[fieldstart:fieldend].replace('%', ''))
    except:
        return None


def time_program(progcmd):
    sp = subprocess.run(
        ['/usr/bin/time', '--verbose', '--'] + progcmd.split(' '),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    if sp.returncode != 0:
        log(str(sp.stdout, 'utf-8'))
        log(str(sp.stderr, 'utf-8'))
        return None

    time_output = str(sp.stderr)

    perf_data = {
        'cpu': get_time_field(time_output, 'Percent of CPU this job got:'),
        'stime': get_time_field(time_output, 'System time (seconds):'),
        'utime': get_time_field(time_output, 'User time (seconds):'),
        'wtime': get_time_field(time_output, 'Elapsed (wall clock) time (h:mm:ss or m:ss):'),
        'mrss': get_time_field(time_output, 'Maximum resident set size (kbytes):'),
        'arss': get_time_field(time_output, 'Average resident set size (kbytes):'),
    }

    if None in perf_data.values():
        # we couldn't parse the output, so the command probably failed
        return None
    else:
        return perf_data






def byte_cat(infile, outfile):
    return f'./byte_cat {infile} {outfile}'

def diabolical_byte_cat(infile, outfile):
    return f'./diabolical_byte_cat {infile} {outfile}'

def reverse_byte_cat(infile, outfile):
    return f'./reverse_byte_cat {infile} {outfile}'

def block_cat(infile, outfile):
    return f'./block_cat 32 {infile} {outfile}'

def reverse_block_cat(infile, outfile):
    return f'./reverse_block_cat 32 {infile} {outfile}'

def random_block_cat(infile, outfile):
    return f'./random_block_cat {infile} {outfile}'

def stride_cat(infile, outfile):
    return f'./stride_cat 1 1024 {infile} {outfile}'


def runtests(tests):
    results = {}
    for testname in tests:
        results[testname] = {}

    infile = '/tmp/infile'
    outfile = '/tmp/outfile'
    silent_shell(f'dd if=/dev/urandom of={infile} bs=512K count=1')

    def suite(makecmd, impl):
        log(f'\033[31mrunning test suite: {impl}\033[0m')
        silent_shell('make clean')
        silent_shell(makecmd)

        for (i, testname) in enumerate(tests):
            log(f'\033[32m{i + 1}. {impl}::{testname}\033[0m')
            silent_shell(f'> {outfile}')
            prog_str = tests[testname](infile, outfile)
            log('-> ' + prog_str)
            results[testname][impl] = time_program(prog_str)


    suite('make -B IMPL=stdio', 'stdio')
    suite('CFLAGS=-DCACHE_SIZE=4096 make -B IMPL=student', 'student')

    metrics = {}
    for testname in tests:
        if type(results[testname]['stdio']) is not dict:
            print('ERROR: stdio program failed. This should not happen, please contact the course staff')
            print(results[testname]['stdio'])
            sys.exit(1)
        if results[testname]['student'] is None:
            log('student program failed')
            metrics[testname] = 'student test failed'
            continue

        stdio_time = results[testname]['stdio']['wtime']
        student_time = results[testname]['student']['wtime']

        if stdio_time == 0.0:
            stdio_time = 0.001

        ratio = student_time / stdio_time
        metrics[testname] = ratio
        log('performance result: %s: stdio=%.2fs, student=%.2fs, ratio=%.2f' \
                % (testname, stdio_time, student_time, student_time / stdio_time))

    log('======= PERFORMANCE RESULTS =======')
    if "--grader" not in sys.argv:
        for (test, ratio) in metrics.items():
            if ratio == 'student test failed':
                print("{}: \033[31;1mfailed\033[0m".format(test))
            elif "byte" in test and ratio <= 10.0:
              print("{}: \033[32;1m{}x\033[0m stdio's runtime".format(test, round(ratio, 2)))
            elif ratio <= 5.0:
              print("{}: \033[32;1m{}x\033[0m stdio's runtime".format(test, round(ratio, 2)))
            else:
              print("{}: \033[31;1m{}x\033[0m stdio's runtime".format(test, round(ratio, 2)))
    else:
        print(json.dumps(metrics, indent=4))

    silent_shell('make clean')
    silent_shell(f'rm -- {infile} {outfile}')

runtests({
    'byte_cat': byte_cat,
#    'diabolical_byte_cat': diabolical_byte_cat,
    'reverse_byte_cat': reverse_byte_cat,
    'block_cat': block_cat,
    'reverse_block_cat': reverse_block_cat,
    'random_block_cat': random_block_cat,
    'stride_cat': stride_cat
})
