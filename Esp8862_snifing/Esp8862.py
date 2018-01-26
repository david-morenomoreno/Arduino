import serial
from elasticsearch import Elasticsearch
import json
import time
import simplejson
from constants import *
from netaddr import *




#Inicializacion elastic_host

es = Elasticsearch()
profile = {
    "mappings": {
        "esp8862": {
            "properties": {
                "date": {
                    "type": "date",
                    "format": "epoch_second"
                },
            }
        },
    }
}


es.indices.create(index="esp8862", ignore=400, body=profile)



while 1:
    elastic = Elasticsearch([{'host': elastic_host, 'port': elastic_port}])

    ser = serial.Serial('/dev/ttyUSB0', 57600)

    try:
        serial_line = (ser.readline().decode('UTF-8')).rstrip()

        print(serial_line)

        if serial_line:
            jsonitem = json.loads(serial_line)
            jsonitem['date'] = int(time.time())

            try:
                mac = EUI(jsonitem['BEdevice'])
                jsonitem['BEmaker'] = mac.oui.registration().org

            except:
                pass

            elastic.index(index='esp8862', doc_type='doc', body=simplejson.dumps(jsonitem))

            print(str(jsonitem))
    except:
        pass

