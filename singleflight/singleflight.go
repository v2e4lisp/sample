package singleflight

import "sync"

type result struct {
	val  interface{}
	err  error
	done chan struct{}
}

type SingleFlight struct {
	fn    func(interface{}) (interface{}, error)
	mu    sync.RWMutex
	cache map[interface{}]*result
}

func New(fn func(interface{}) (interface{}, error)) *SingleFlight {
	return &SingleFlight{
		fn:    fn,
		cache: make(map[interface{}]*result),
	}
}

func (f *SingleFlight) Do(key interface{}, v interface{}) (interface{}, error) {
	f.mu.RLock()
	if r, ok := f.cache[key]; ok {
		f.mu.RUnlock()
		<-r.done
		return r.val, r.err
	}
	f.mu.RUnlock()
	f.mu.Lock()
	r := &result{
		done: make(chan struct{}),
	}
	f.cache[key] = r
	f.mu.Unlock()
	r.val, r.err = f.fn(v)
	close(r.done)
	return r.val, r.err
}
