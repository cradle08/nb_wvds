#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys
import csv
import os

header = ['文件名','版本号','Git版本号','版本日期','SHA256摘要']
content = []
outfile = 'version.csv'

if __name__ == '__main__':
    if len(sys.argv) < 6:
        print "Usage:"
        print "  version.py filename verno commit verdate digest"
        sys.exit(0)

    fname = sys.argv[1]
    verno = sys.argv[2]
    commit = sys.argv[3]
    verdate = sys.argv[4]
    digest = sys.argv[5]

    found = False
    if os.path.exists(outfile):
        with open(outfile, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                if row[header[0]] == fname:
                    row[header[1]] = verno
                    row[header[2]] = commit
                    row[header[3]] = verdate
                    row[header[4]] = digest
                    found = True
                    print 'UPD: %s,%s,%s,%s,%s' % (fname,verno,commit,verdate,digest)
                content.append(row)

    if not found:
        print 'ERR: No row for %s, create new row' % (fname)
        row = {}
        row[header[0]] = fname
        row[header[1]] = verno
        row[header[2]] = commit
        row[header[3]] = verdate
        row[header[4]] = digest
        print 'ADD: %s,%s,%s,%s,%s' % (fname,verno,commit,verdate,digest)
        content.append(row)

    with open(outfile, 'w') as csvfile:
        writer = csv.DictWriter(csvfile, header)
        writer.writeheader()
        for row in content:
            writer.writerow(row)
    print '%s written' % (os.path.abspath(outfile))
