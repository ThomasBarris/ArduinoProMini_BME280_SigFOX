ORG_EMAIL   = "@outlook.com"
FROM_EMAIL  = "mail" + ORG_EMAIL
FROM_PWD    = "app password"
MAIL_SERVER = "imap-mail.outlook.com"
PATH = '/home/thomas/sigfox/'
LOGFILE = 'sigfox.log'

MQTT_SERVER     = "xxx.xxx.xxx.xxx"
MQTT_SERVERPORT = 1883
MQTT_USERNAME   = "username"
MQTT_KEY        = "password"

import imaplib
import email
import datetime
import os.path
import time
import sys
import paho.mqtt.client as paho

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
#    client.subscribe("$SYS/#")

def mqtt_connect():
  ts = timestamp()
  print (ts + ' : ' + 'Connecting to MQTT Server', file=f)
  client = paho.Client("sigfox")
  client.username_pw_set(MQTT_USERNAME, MQTT_KEY)
  client.on_connect = on_connect
  client.connect(MQTT_SERVER, port=MQTT_SERVERPORT)
#  client.loop_forever()
  ts = timestamp()
  print (ts + ' : ' + 'Connecting to MQTT Server...done', file=f)


def mqtt_disconnect():
  print('Disconnecting from MQTT broker...')
  client.loop_stop()
  client.disconnect()

def mqtt_publish(mtemp,mhum,mpress):
  client.publish("/hiddensee/inselblick/temperatur", mtemp)
  client.publish("/hiddensee/inselblick/humidity", mhum)
  client.publish("/hiddensee/inselblick/pressure", mpress)


def check_logfile_or_create():
  file_path = PATH + LOGFILE
  if not os.path.exists(file_path) == True :
    open(file_path, 'a').close() 

def fetch_mail_body (uid):
  fetchmail_r, data = mail.uid('fetch', uid, '(RFC822)')
  raw_email = data[0][1] # here's the body, which is raw text of the whole email
                       # including headers and alternate payloads

  #decoding the message
  raw_email = raw_email.decode("utf-8")

  email_message = email.message_from_string(raw_email)

  for part in email_message.walk():
    # each part is a either non-multipart, or another multipart message
    # that contains further parts... Message is organized like a tree
    if part.get_content_type() == 'text/plain':
      payload = part.get_payload().split() # prints the raw text        
     # print (payload)
      if payload[0]=="SIGFOX":
        sigfox_ts = ( ( time.strftime('%m/%d/%Y %H:%M:%S',  time.gmtime(int(payload[1])))) + ' : ' + payload[2])

  if len(payload[2]) < 12:
    ts = timestamp()
    print (ts + ' : ' + 'ERROR payload to short : ' + payload[2], file=f)
    return 0,0,0,0,0
  hpayload = payload[2]
#  print (hpayload)
  htemp = int (hpayload[0:4], 16)
  hhum  = int (hpayload[4:8], 16)
  #print (hpayload[4:7])
  hpress= int (hpayload[8:12], 16)
  temp = (htemp-500)/10
  hum = hhum/10
  press = hpress/10
  ts = timestamp()
  print (ts + ' : ' + 'Converted Payload', file=f )
  print (sigfox_ts + ": " + str(temp) + " " + str(hum) + " " + str(press))
  return 1,sigfox_ts,temp,hum,press


def timestamp():
  log_timestamp = ('{:%Y-%m-%d %H:%M:%S}'.format(datetime.datetime.now()))
  return log_timestamp



check_logfile_or_create()
file_path = PATH + LOGFILE
f = open(file_path, 'a')

ts = timestamp()
print ( ts + ' : ' + '***************STARTING**************', file=f)

ts = timestamp()
print (ts + ' : ' + 'Connecting to MQTT Server', file=f)
client = paho.Client("sigfox")
client.username_pw_set(MQTT_USERNAME, MQTT_KEY)
client.on_connect = on_connect
client.connect(MQTT_SERVER, port=MQTT_SERVERPORT)
#  client.loop_forever()
ts = timestamp()
print (ts + ' : ' + 'Connecting to MQTT Server...done', file=f)



mail = imaplib.IMAP4_SSL(MAIL_SERVER)
login_r, d1 = mail.login(FROM_EMAIL, FROM_PWD)
ts = timestamp()
print ( ts + ' : ' + 'Login to ' + MAIL_SERVER + ' : ' + login_r, file=f)
mail.list()

# Out: list of "folders" aka labels in gmail.
select_r,d2 =  mail.select("Inbox") # connect to inbox.
ts = timestamp()
print (ts + ' : ''Select Inbox : ' + select_r, file=f)

fetchuid_r, data = mail.uid('search', None, "ALL") # search and return uids
ts = timestamp()
print (ts + ' : ' + 'Fetching Message UIDs : ' + fetchuid_r, file=f)


email_ids  = data[0].split() #data is a space separated list
#print (email_ids)

# if the inbox is empty, the latest_email i.e. [-1] raise an IndexError
try:
  latest_email_uid = email_ids[-1] # get the latest email
  ts = timestamp()
  print (ts + ' : ' + 'latest email ID : ' + str(latest_email_uid),file=f)
except IndexError:
  ts = timestamp()
  print (ts + ' : ' + 'email list empty', file=f)
  # leave the program, cleaning up
  mail.logout()
  mqtt_disconnect()
  f.close()
  sys.exit()

#mqtt_connect()

# Loop through the array of mails
ts = timestamp()
print (ts + ' : ' + 'Fetching mails & extracting values  ', file=f)

for i in email_ids:
  result,sigfox_ts,temp,hum,press = fetch_mail_body(i)
  if result == 1:
    ts = timestamp()
    print (ts + ' : ' + 'Sending values to MQTT broker  ', file=f)
    mqtt_publish(temp,hum,press)
    mail.uid('STORE', i , '+FLAGS', '(\Deleted)') 
mail.expunge()  

ts = timestamp()
print (ts + ' : ' + 'closing program  ', file=f)
mail.logout()
mqtt_disconnect()
f.close()

# note that if you want to get text content (body) and the email contains
# multiple payloads (plaintext/ html), you must parse each message separately.
# use something like the following: (taken from a stackoverflow post)
def get_first_text_block(self, email_message_instance):
    maintype = email_message_instance.get_content_maintype()
    if maintype == 'multipart':
        for part in email_message_instance.get_payload():
            if part.get_content_maintype() == 'text':
                return part.get_payload()
    elif maintype == 'text':
        return email_message_instance.get_payload()

#mail.logout()