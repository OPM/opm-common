#!/usr/bin/env python

import sys
from os.path import isdir, join
import sunbeam
from datetime import datetime as dt

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

def swof_krw(ecl):
    assert('SWOF' in ecl.table)
    krw  = ecl.table['SWOF', 'KRW']
    krow = ecl.table['SWOF', 'KROW']
    pcow = ecl.table['SWOF', 'PCOW']

    print('SWOF\tKRW\tKROW\tPCOW')
    for i in range(21):
        print('%.2f\t%.4f\t%.4f\t%.4f' % (i/20.0, krw(i/20.0), krow(i/20.0), pcow(i/20.0)))

def main():
    es = parse(join(opmdatadir(), 'norne/NORNE_ATW2013.DATA'))
    sc = es.schedule
    wp = sc.wells[23] # producer
    wi = sc.wells[20] # injector at ts 100
    fn = es.faultNames()
    f0 = fn[0]
    fl = es.faults()[f0]
    print('state:     %s' % es)
    print('schedule:  %s' % sc)
    print('the grid:  %s' % es.grid())
    print('at timestep 100 (%s)' % sc.timesteps[100])
    print('prod well: %s' % wp)
    print('inj  well: %s' % wi)
    print('pos:       %s' % list(wp.pos()))
    print('fault:     %s' % f0)
    print('           comprised of %d cells' % len(fl))
    swof_krw(es)

if __name__ == '__main__':
    global OPMDATA_DIR
    OPMDATA_DIR = '../../opm-data'
    if haveopmdata():
        print('Found norne, parsing ...')
        main()
    else:
        print('Need to have path "%s" or give opm-data as argument' % OPMDATA_DIR)
