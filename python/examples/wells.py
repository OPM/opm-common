import sunbeam

def main():
    es = sunbeam.parse('../tests/spe3/SPE3CASE1.DATA')
    sc = es.schedule
    wp = sc.wells[0] # producer
    wi = sc.wells[1] # injector
    print('state:     %s' % es)
    print('schedule:  %s' % sc)
    print('prod well: %s' % wp)
    print('inj  well: %s' % wi)
    for i in range(len(sc.timesteps)):
        if not wp.isproducer(i) or wp.isinjector(i):
            print('wp is not producer in step %s' % sc.timesteps[i])
        if not wi.isinjector(i) or wi.isproducer(i):
            print('wi is not injector in step %s' % sc.timesteps[i])

if __name__ == '__main__':
    main()
