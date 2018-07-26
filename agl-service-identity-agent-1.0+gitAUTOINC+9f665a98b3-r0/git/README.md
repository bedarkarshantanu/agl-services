agl-identity-agent
==================

**agl-identity-agent** is an OpenID Connect Identity service/binding
for AGL (Automotive Grade Linux).

Overview
--------

The binding currently reads its configuration from a file.

Then, it starts a GATT Bluetooth LE "User Data" service.

When the email characteristic is written, the server is queried to
get the data associated with the key (keytoken=...) for the
current vehicle (vin=...). The key is the email value set.
This is the login process.

An event notifying that a user logged is sent to applications.

The configuration file
----------------------

The configuration file is a JSON file residing in one of the 
following places:

 - ID/config.json
 - /etc/agl/identity-agent-config.json
 - CWD/config.json

Where ID is the installation directory and CWD is the
current working directory.

The JSON looks like:

```json
{
 "endpoint": "https://agl-graphapi.forgerocklabs.org/getuserprofilefromtoken",
 "vin": "4T1BF1FK5GU260429",
 "autoadvise": true,
 "delay": 5,
 "idp": {
     "authorization_endpoint": "",
     "token_endpoint": "https://agl-am.forgerocklabs.org:8043/openam/oauth2/stateless/access_token"
   },
 "appli": {
     "authorization": "Basic c3RhdGVsZXNzOnBhc3N3b3JkMg==",
     "username": "bjensen",
     "password": "Passw0rd",
     "scope": "openid profile email cn sn givenName ou mail postalAddress departmentNumber physicalDeliveryOfficeName facsimileTelephoneNumber"
   }
}
```

Where:

 - *delay* is the delay where server request is ignored when a previous request
   is started since sthis dealy
 - *autoadvise* is a boolean indicating whether the binding must start
   the service automatically at initialisation
 - *vin* is the vehicule identification number
 - *endpoint* is the enpoint to be queried for getting user data
 - *idp* describes the OAuth2/OpenId Connect IDP (identity provider)
 - *appli* describes the data of the application for the IDP

Not setting *idp* or *appli* implies that no token is queried.

Verbs of API
------------

### agl-identity-agent/advise

Starts offering service on BT interface (hci0).

No argument needed.

### agl-identity-agent/unadvise

Stops offering service on BT interface (hci0).

No argument needed.

### agl-identity-agent/subscribe

Subscribes to event notifications.

No argument needed.

### agl-identity-agent/unsubscribe

Unsubscribes from event notifications.

No argument needed.

### agl-identity-agent/login

Not implemented, always fails.

No argument needed.

### agl-identity-agent/logout

Logout from the current identity.

No argument needed.

### agl-identity-agent/get

Returns the data for the current identity.

No argument needed.

Events of API
-------------

The binding sends the event *agl-identity-agent/event*.

This event signals logins and logouts. It has 2
fields: *eventName* and *accountId*.

For login events, the *eventName* is the string *login*
and the *accountId* is the string identifying the account.

Example of login event:

```json
{
  "eventName": "login",
  "accountId": "farfoll"
}
```

For login events, the *eventName* is the string *logout*
and the *accountId* is the string *null*.

Example of logout event:

```json
{
  "eventName": "logout",
  "accountId": "null"
}
```

OAuth2 & OpenId Connect integration
-----------------------------------

When the fields 'appli' and 'idp' are set, the agent uses the
related data to query an access token for accessing the account
data using the flow _Resource Owner Password Credentials Grant_.

