import subprocess, time, os, httplib, random, threading, multiprocessing

import toolspath
from testing import Test
from testing.test import Failure

class ArgTest(Test):
   timeout = 2
   point_value = 2

   def run_argtest(self, port=0, threads=1, buffers=1, n=None):
      if (port == 0):
         port = random.randint(5000, 10000)
      self.log("Starting server on port " + str(port))
      args = ["server", str(port), str(threads), str(buffers)]
      if n is not None:
         args.append(str(n))
      serverProc = self.startexe(args)
      time.sleep(0.5)
      if (serverProc.poll() == None):
         serverProc.kill()
         self.fail("server should have exited")

class ArgPort(ArgTest):
   name = "argport"
   description = "invalid port number"

   def run(self):
      self.run_argtest(port=22)
      self.done()

class ServerTest(Test):
   timeout = 30
   point_value = 5

   def quiet_get(self, conn, item):
      try:
         conn.request('GET', item)
         return True
      except Exception as e:
         self.fail('unable to send request to server. it may have crashed.')
         return False

   def client_run(self, conn):
      try:
         response = conn.getresponse()
         msg = response.read()
         conn.close()
         if (len(msg) == 0):
            self.fail("missing body in response")
      except Exception as inst:
         self.fail("Client failed with error: " + str(inst))
         #raise Failure("Client failed with error: " + str(inst))

   def run_server(self, threads, buffers, n=None):
      minport = 5000
      maxport = 10000
      root = os.path.join(self.test_path, "files")
      for i in range(5):
         port = random.randint(minport, maxport)
         self.log("Starting server on port " + str(port))
         args = ["server", str(port), str(threads), str(buffers)]
         if n is not None:
            args.append(str(n))
         serverProc = self.startexe(args, cwd=root)
         time.sleep(0.2)
         # wait for server to respond
         serverProc.poll()
         if serverProc.returncode is None:
            conn = httplib.HTTPConnection("localhost", port, timeout=2)
            self.quiet_get(conn, "/home.html")
            try:
               response = conn.getresponse()
               response.read()
            except Exception:
               pass
            serverProc.poll()
            if serverProc.returncode is None:
               time.sleep(0.1)
               self.port = port
               self.serverProc = serverProc
               return serverProc
         try:
            serverProc.kill()
         except:
            pass
      raise Failure("Could not start server")

class Basic(ServerTest):
   name = "basic"
   description = "check if the webserver can serve requests"
   point_value = 4

   def client1_run(self):
      try:
         conn = httplib.HTTPConnection("localhost", self.port, timeout=10)
         print("Connected")
         self.quiet_get(conn, "/home.html")
         print("Requested")
         response = conn.getresponse()
         print("Got response")

         msg = response.read()

         print("Read response")
         if (len(msg) == 0):
            self.fail("missing body in response")

         conn.close()
      except Exception as inst:
         self.fail("Client failed with error: " + str(inst))

   threads = 1
   buffers = 1

   def run(self):
      serverProc = self.run_server(self.threads, self.buffers)
      self.client1_run()
      serverProc.kill()
      self.done()

class Pool2(ServerTest):
   name = "pool2"
   description = "check if using a fixed size thread pool"
   def run(self):
      serverProc = self.run_server(threads=3, buffers=8)
      conns = list()

      for i in range(6):
         conns.append(httplib.HTTPConnection("localhost", self.port, timeout=10))
         self.quiet_get(conns[-1], "/output.cgi?1")
         time.sleep(0.1)

      try:
         conns[0].getresponse()
         conns[1].getresponse()
         conns[2].getresponse()
         t1 = time.time()

         conns[3].getresponse()
         conns[4].getresponse()
         conns[5].getresponse()
         t2 = time.time()
         if t2 < t1 + 1:
            self.fail("not using a fixed size thread pool (size 2)")
      except Exception as inst:
         self.fail("Client failed with error: " + str(inst))

      serverProc.kill()
      self.done()


class Locks(ServerTest):
   name = "locks"
   description = "many concurrent requests to test locking"
   threads = 8
   buffers = 16
   num_clients = 20
   loops = 20
   requests = ["/home.html", "/output.cgi?0.3", "/in"]
   def many_reqs(self):
      for i in range(self.loops):
         for request in self.requests:
            if(self.is_failed()):
               return
            conn = httplib.HTTPConnection("localhost", self.port, timeout=6)
            if self.quiet_get(conn, request):
               self.client_run(conn)

   def run(self):
      serverProc = self.run_server(threads=self.threads, buffers=self.buffers)
      clients = [threading.Thread(target=self.many_reqs) for i in range(self.num_clients)]
      for client in clients:
         client.start()
      for client in clients:
         client.join()
      if (serverProc.poll() != None):
         self.fail("server exited or crashed")
      else:
         serverProc.kill()
      self.done()

class Locks3(Locks):
   name = "locks3"
   threads = 64
   buffers = 128
   num_clients = 50
   loops = 6 
   requests = ["/home.html", "/output.cgi?0.3", "/in", "/output.cgi?0.2"]
   timeout = 60

class Equal2(Locks):
   name = "equal2"
   description = "equal buffers and threads"
   threads = 32
   buffers = 32
   num_clients = 32
   loops = 20

class Fewer(Locks):
   name = "fewer"
   description = "fewer buffers than threads"
   threads = 16
   buffers = 8
   num_clients = 20
   loops = 20

class Light(Locks):
   name = "light"
   description = "light request load"
   threads = 20
   buffers = 10
   num_clients = 5
   loops = 20
   requests = ["/home.html", "/output.cgi?0.1", "/in", "/output.cgi?0.02"]

class Single(Locks):
   name = "single"
   description = "single thread serving many requests"
   threads = 1
   buffers = 30
   num_clients = 25
   loops = 20
   requests = ["/home.html", "/in"]

class BusyWait(ServerTest):
   name = "nobusywait"
   description = "test to make sure you are not busy-waiting"
   point_value = 15
   def run(self):
      serverProc = self.run_server(threads=10, buffers=1)
      time.sleep(5);
      cmd = ['ps', '--no-headers', '-o', 'time', str(serverProc.pid)]
      ps = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]

      hr, min, sec = tuple(map(int, ps.split(':')))
      total = sec + (60 * (min + (60 * hr)))
      print 'idle server spent %d seconds running on CPU' % (total,)

      if total != 0:
         self.fail('idle server should spend 0 seconds running on CPU')
      serverProc.kill()
      self.done()

test_list = [Basic, ArgPort, BusyWait, Pool2, Single, Light, Locks, Locks3, Equal2, Fewer]
