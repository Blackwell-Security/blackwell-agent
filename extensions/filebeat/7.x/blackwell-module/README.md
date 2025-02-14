# Blackwell Filebeat module

## Hosting

The Blackwell Filebeat module is hosted at the following URLs

- Production:
  - https://packages.wazuh.com/4.x/filebeat/
- Development:
  - https://packages-dev.blackwell.com/pre-release/filebeat/
  - https://packages-dev.blackwell.com/staging/filebeat/

The Blackwell Filebeat module must follow the following nomenclature, where revision corresponds to X.Y values

- blackwell-filebeat-{revision}.tar.gz

Currently, we host the following modules

|Module|Version|
|:--|:--|
|blackwell-filebeat-0.1.tar.gz|From 3.9.x to 4.2.x included|
|blackwell-filebeat-0.2.tar.gz|From 4.3.x to 4.6.x included|
|blackwell-filebeat-0.3.tar.gz|4.7.x|
|blackwell-filebeat-0.4.tar.gz|From 4.8.x to current|


## How-To update module tar.gz file

To add a new version of the module it is necessary to follow the following steps:

1. Clone the blackwell/blackwell repository
2. Check out the branch that adds a new version
3. Access the directory: **extensions/filebeat/7.x/blackwell-module/**
4. Create a directory called: **blackwell**

```
# mkdir blackwell
```

5. Copy the resources to the **blackwell** directory

```
# cp -r _meta blackwell/
# cp -r alerts blackwell/
# cp -r archives blackwell/
# cp -r module.yml blackwell/
```

6. Set **root user** and **root group** to all elements of the **blackwell** directory (included)

```
# chown -R root:root blackwell
```

7. Set all directories with **755** permissions

```
# chmod 755 blackwell
# chmod 755 blackwell/alerts
# chmod 755 blackwell/alerts/config
# chmod 755 blackwell/alerts/ingest
# chmod 755 blackwell/archives
# chmod 755 blackwell/archives/config
# chmod 755 blackwell/archives/ingest
```

8. Set all yml/json files with **644** permissions

```
# chmod 644 blackwell/module.yml
# chmod 644 blackwell/_meta/config.yml
# chmod 644 blackwell/_meta/docs.asciidoc
# chmod 644 blackwell/_meta/fields.yml
# chmod 644 blackwell/alerts/manifest.yml
# chmod 644 blackwell/alerts/config/alerts.yml
# chmod 644 blackwell/alerts/ingest/pipeline.json
# chmod 644 blackwell/archives/manifest.yml
# chmod 644 blackwell/archives/config/archives.yml
# chmod 644 blackwell/archives/ingest/pipeline.json
```

9. Create **tar.gz** file

```
# tar -czvf blackwell-filebeat-0.4.tar.gz blackwell
```

10. Check the user, group, and permissions of the created file

```
# tree -pug blackwell
[drwxr-xr-x root     root    ]  blackwell
├── [drwxr-xr-x root     root    ]  alerts
│   ├── [drwxr-xr-x root     root    ]  config
│   │   └── [-rw-r--r-- root     root    ]  alerts.yml
│   ├── [drwxr-xr-x root     root    ]  ingest
│   │   └── [-rw-r--r-- root     root    ]  pipeline.json
│   └── [-rw-r--r-- root     root    ]  manifest.yml
├── [drwxr-xr-x root     root    ]  archives
│   ├── [drwxr-xr-x root     root    ]  config
│   │   └── [-rw-r--r-- root     root    ]  archives.yml
│   ├── [drwxr-xr-x root     root    ]  ingest
│   │   └── [-rw-r--r-- root     root    ]  pipeline.json
│   └── [-rw-r--r-- root     root    ]  manifest.yml
├── [drwxr-xr-x root     root    ]  _meta
│   ├── [-rw-r--r-- root     root    ]  config.yml
│   ├── [-rw-r--r-- root     root    ]  docs.asciidoc
│   └── [-rw-r--r-- root     root    ]  fields.yml
└── [-rw-r--r-- root     root    ]  module.yml
```

11. Upload file to development bucket
