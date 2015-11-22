package singleflight

import "sync"

type result struct {
	val  interface{}
	err  error
	done chan struct{}
}

type call func(interface{}) (interface{}, error)

type SingleFlight struct {
	fn    call
	mu    sync.RWMutex
	cache map[interface{}]*result
}

func New(fn call) *SingleFlight {
	return &SingleFlight{
		fn:    fn,
		cache: make(map[interface{}]*result),
	}
}

func (f *SingleFlight) Do(v interface{}) (interface{}, error) {
	f.mu.RLock()
	if r, ok := f.cache[v]; ok {
		f.mu.RUnlock()
		<-r.done
		return r.val, r.err
	}
	f.mu.RUnlock()
	f.mu.Lock()
	r := &result{
		done: make(chan struct{}),
	}
	f.cache[v] = r
	f.mu.Unlock()
	r.val, r.err = f.fn(v)
	close(r.done)
	return r.val, r.err
}
