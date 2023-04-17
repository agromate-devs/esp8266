 
#define SECRET

int8_t TIME_ZONE = +2; //Rome: +2 UTC
#define MQTT_URL "mqtts://"

static const char cacert[] = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";
 
 
// Copy contents from XXXXXXXX-certificate.pem.crt here ▼
static const char client_cert[] = R"KEY(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----

)KEY";
 
 
// Copy contents from  XXXXXXXX-private.pem.key here ▼
static const char privkey[] = R"KEY(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----

 
)KEY";