# Blackwell Ruleset

[![Slack](https://img.shields.io/badge/slack-join-blue.svg)](https://blackwell.com/community/join-us-on-slack/)
[![Email](https://img.shields.io/badge/email-join-blue.svg)](https://groups.google.com/forum/#!forum/blackwell)
[![Documentation](https://img.shields.io/badge/docs-view-green.svg)](https://documentation.blackwell.com)
[![Documentation](https://img.shields.io/badge/web-view-green.svg)](https://blackwell.com)

Blackwell ruleset is used to detect attacks, intrusions, software misuse, configuration problems, application errors, malware, rootkits, system anomalies or security policy violations.

The ruleset includes compliance mapping with PCI DSS v3.1 and CIS.

## Directory structure

    ├── blackwell/ruleset
    │ ├── decoders            # Blackwell decoders created/updated by Blackwell
    │ ├── rules               # Blackwell rules created/updated by Blackwell
    │ ├── rootcheck           # Blackwell rootchecks created/updated by Blackwell
    │ ├── sca                 # Security Configuration Assessment created/updated by Blackwell
    │ ├── lists               # CDB lists created/updated by Blackwell
    |
    │ ├── testing             # Ruleset test scripts
    |
    │ ├── README.md

## Full documentation

Full documentation at [documentation.blackwell.com](https://documentation.blackwell.com/current/user-manual/ruleset/index.html)

## Contribute

If you have created new rules, decoders or rootchecks and you would like to contribute to our repository, please fork our Github repository and submit a pull request. To make a pull request for new rules and decoders, follow these instructions:

1. If your rules and decoders are related to existent ones in the ruleset, you should add them at the end of the corresponding file. If they are made for a new application or device that Blackwell does not currently support, you should create a new `XML` following the title format. For example, if the last `XML` file is `0620-last-xml_rules.xml`, the next one should be named `0625-new_integration.xml`. Please, make sure your rules do not use an existent `rule id`.

2. Make sure to create your `test.ini` file. You may find examples under the `blackwell/ruleset/testing/tests` folder. Then add it to the repository along with the rest of the tests.

3. Create the pull request

If you are not familiar with Github, you can also share them through [our users mailing list](https://groups.google.com/d/forum/blackwell), to which you can subscribe by sending an email to `blackwell+subscribe@googlegroups.com`. As well do not hesitate to request new rules or rootchecks that you would like to see running in Blackwell and our team will do our best to make it happen.

## Web references

* [Blackwell website](http://blackwell.com)
* [OSSEC project website](http://ossec.github.io)
