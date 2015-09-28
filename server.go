package socks4

import (
	"bufio"
	"io"
	"log"
	"net"
	"sync"
)

const (
	SocksVersion4 = byte(4)
	Connect       = byte(1)
	Bind          = byte(2)
)

var (
	RequestGranted        = []byte{0, 90, 0, 0, 0, 0, 0, 0}
	RequestRejected       = []byte{0, 91, 0, 0, 0, 0, 0, 0}
	RequestRejectedUserId = []byte{0, 92, 0, 0, 0, 0, 0, 0}
)

var (
	noAuth     = func(_ net.Addr, _ net.TCPAddr, _ []byte) bool { return true }
	noFireWall = func(_ net.Addr) bool { return true }
)

type Server struct {
	Auth     func(net.Addr, net.TCPAddr, []byte) bool
	FireWall func(net.Addr) bool
}

func (s *Server) setDefaults() {
	if s.Auth == nil {
		s.Auth = noAuth
	}
	if s.FireWall == nil {
		s.FireWall = noFireWall
	}
}

func (s *Server) Serve(addr string) error {
	s.setDefaults()
	l, err := net.Listen("tcp", addr)
	if err != nil {
		return err
	}

	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println(err)
		}
		log.Println("Received connection", conn.RemoteAddr())
		go s.handleConn(conn)
	}

	return nil
}

func (s *Server) handleConn(conn net.Conn) {
	defer func() {
		log.Println("Closed connection", conn.RemoteAddr())
		conn.Close()
	}()
	if !s.FireWall(conn.RemoteAddr()) {
		return
	}
	bufConn := bufio.NewReader(conn)
	ver, err := bufConn.ReadByte()
	if err != nil {
		log.Println("Failed to get version number")
		return
	}
	if ver != SocksVersion4 {
		log.Println("Version number is invalid")
		return
	}

	cmd, err := bufConn.ReadByte()
	if err != nil {
		log.Println("Failed to get command")
		return
	}

	switch cmd {
	case Connect:
		s.handleConnect(conn, bufConn)
	case Bind:
		s.handleBind(conn, bufConn)
	default:
		s.handleOther(conn, bufConn)
	}
}

func (s *Server) handleConnect(conn net.Conn, bufConn *bufio.Reader) {
	addr := make([]byte, 6) // 2 + 4
	_, err := io.ReadAtLeast(bufConn, addr, 6)
	if err != nil {
		log.Println("Failed to read address")
		conn.Write(RequestRejected)
		return
	}
	tcpAddr := net.TCPAddr{
		IP:   net.IPv4(addr[2], addr[3], addr[4], addr[5]),
		Port: (int(addr[0]) << 8) | int(addr[1]),
	}

	userId, err := bufConn.ReadBytes(0)
	if err != nil {
		log.Println("Failed to read user id")
		conn.Write(RequestRejected)
		return
	}
	if !s.Auth(conn.RemoteAddr(), tcpAddr, userId) {
		log.Printf("user id(%v) is authorized", userId)
		conn.Write(RequestRejectedUserId)
		return
	}

	log.Println("Dialing", tcpAddr.String(), "for", conn.RemoteAddr())
	dstConn, err := net.DialTCP("tcp", nil, &tcpAddr)
	if err != nil {
		log.Println("Failed to connect to destination", err)
		conn.Write(RequestRejected)
		return
	}
	defer dstConn.Close()
	conn.Write(RequestGranted)

	var wg sync.WaitGroup
	wg.Add(2)
	go func() { io.Copy(dstConn, conn); wg.Done() }()
	go func() { io.Copy(conn, dstConn); wg.Done() }()
	wg.Wait()
}

func (s *Server) handleBind(conn net.Conn, bufConn *bufio.Reader) {
	log.Println("Bind is not supported")
	conn.Write(RequestRejected)
}

func (s *Server) handleOther(conn net.Conn, bufConn *bufio.Reader) {
	log.Println("Invalid Command")
	conn.Write(RequestRejected)
}
