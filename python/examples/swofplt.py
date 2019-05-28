#!/usr/bin/env python

import sys
from os.path import isdir, join
import sunbeam
from datetime import datetime as dt

import numpy as np
import matplotlib.pyplot as plt


def plotswof(ecl):
    assert('SWOF' in ecl.table)
    krw  = ecl.table['SWOF', 'KRW']
    krow = ecl.table['SWOF', 'KROW']
    pcow = ecl.table['SWOF', 'PCOW']

    swofl = [x/20.0       for x in range(21)]
    krwl  = [krw(x/20.0)  for x in range(21)]
    krowl = [krow(x/20.0) for x in range(21)]
    pcowl = [pcow(x/20.0) for x in range(21)]

    plt.figure(1)
    plt.plot(swofl, krwl, label = 'KRW')
    plt.plot(swofl, krowl, label = 'KROW')
    plt.legend()
    plt.show()
    plt.figure(2)
    plt.plot(swofl, pcowl, label = 'Water-oil capillary pressure')
    plt.legend()
    plt.show()



def opmdatadir():
    global OPMDATA_DIR
    if isdir(OPMDATA_DIR):
        return OPMDATA_DIR
    if len(sys.argv) < 2:
        return None
    d = sys.argv[1]
    if isdir(d) and isdir(join(d, 'norne')):
        return d
    return None

def haveopmdata():
    return opmdatadir() is not None

def parse(fname):
    s = dt.now()
    es = sunbeam.parse(fname, ('PARSE_RANDOM_SLASH', sunbeam.action.ignore))
    e = dt.now()
    print('Parsing took %s sec' % (e - s).seconds)
    return es


def main():
    es = parse(join(opmdatadir(), 'norne/NORNE_ATW2013.DATA'))
    plotswof(es)

if __name__ == '__main__':
    global OPMDATA_DIR
    OPMDATA_DIR = '../../opm-data'
    if haveopmdata():
        print('Found norne, parsing ...')
        main()
    else:
        print('Need to have path "%s" or give opm-data as argument' % OPMDATA_DIR)
