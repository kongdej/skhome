import microgear.client as microgear
import sys


if len(sys.argv) > 1:
  # NETPIE appid and apikeys
  appid = "SKHOME"
  gearkey = "VXM0JO7qieJFU4u"
  gearsecret =  "tB9D6qn9poYC2FEgFexxjMIDD"

  microgear.create(gearkey,gearsecret,appid,{'debugmode': False})

  def connection():
    print "Now I am connected with netpie"

  def subscription(topic,message):
    print topic+"="+message
      
  def disconnect():
    print "disconnect is work"

  microgear.setalias("strw_test")
  microgear.on_connect = connection
  microgear.on_message = subscription
  microgear.on_disconnect = disconnect
  microgear.subscribe("/strawberry_watering");
  microgear.connect(False)


  delaytime = sys.argv[1]

  microgear.publish("/strawberry_watering",delaytime)
else:
  print "Please enter delay time.!!"