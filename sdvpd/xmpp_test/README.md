# Application arguments

./xmpp_test sdvp_ctrl_java1@sdvp 123456 localhost


# XMPP server requirements
In SDVP Openfire is used as XMPP server. But all the requierements could be
satisfied by another XMPP server as the underlaying

## Domain
When installing a XMPP server a domain is requested. Here the domain 'sdvp' is 
used and all users are created with this domain.

Using a wrong domain for the client will result in a: 
'SASL authentication DIGEST-MD5 failed: not-authorized'

## Restrict writing
It should not be posible to write to clients not on a clients roster!

## Rosters
For testing the clients rosters must be set up correctly with the following 
in place:

* Two clients
    * One for acting as Controller and one for acting as Service Provider.
    * Controller
        * Username: sdvp_ctrl_java1@sdvp
        * Password: 123456
        * Roster:
            * Contact: sdvp_sp_java1@sdvp
                * Group: ServiceProviders
                * Subscription: Both
    * Service Procider
        * Username: sdvp_sp_java1@sdvp
        * Password: 123456
        * Roster:
            * Contact: sdvp_ctrl_java1@sdvp
                * Group: Controllers
                * Subscription: Both

### Importing Rosters
Using OpenFire it is posible to install an plugin which makes it posible to 
export and import rosters for a whole system.

The plugin is called 'User Import Export' and can be installed from OpenFires
Plugins page. 

Import the file 'openfire_users.xml' which contains the two 

*Note: If exporting be aware of that the admin account is also exported!*

