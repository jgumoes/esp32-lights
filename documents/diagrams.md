# Diagrams

some diagrams i'm using to design this app. these are more for design purposes, so not necessarily accurate

## Configs

### Config File Structure

0-1: Metadata reservation size - 2 complimentary bytes indicating the number of metadata packets that can be stored in the metadata reservation. byte 0 is the size, byte 1 is ~size, therefore ([0] & [1] == 0) and ([0] xor [1] == ~0). once set, they should never change, so if they do not match, data has not been set. also, if the project is updated to include new config types that a device doesn't need, the device will still be able to parse it's stored data

2-metaEnd: Metadata reservation - metaEnd = 2 + (Metadata reservation size) * (Metadata packet size)

metaEnd-EOF: config packet reservation

### Config Storage Class construction process

```mermaid
sequenceDiagram
# ConfigStorage construction process

participant iuser as IConfigUser
participant val as validator
participant config as ConfigStorageClass
participant map as ConfigMap
participant meta as MetadataMapClass
participant storage as StorageAdapter

config->>storage: setAccessLock
activate storage
storage->>storage: sets lock

loop ConfigUser constructors
    iuser->>config: register
    config->>val: set validation
    config->>map: set user reference and default config
end

note over config: loadAllConfigs()

config->>storage: load metadata size
storage->>config: 

opt Size bytes not set (this is dumb delete it)
    %% config->>storage: what type of storage are you?
    %% note over config,storage: (map size)*(is EEPROM ? 2 : 1)
    config->>storage: set size bytes
    %% TODO: check that presumed size doesn't overlap with other reservation
end

loop parse metadata
    config->>storage: load metadata
    config->>map: is ID in map?
    config->>meta: is metadata valid?
    config->>storage: load config
    config->>val: is config valid?
    config->>meta: valid metadata found
    config->>iuser: got a config for you
end

config->>storage: close and release lock

deactivate storage
```

### writing configs

```mermaid
sequenceDiagram
# set new config sequence

participant start as Network Recipient
participant configs as ConfigStorageClass
participant meta as MetadataMapClass
participant adapter as StorageAdapter
participant val as validator
participant iuser as IConfigUser

start-->>start: does the packet CRC check out?
start->>configs: hey i've got this new config

critical Valid the new config
    configs-->>configs: is the ID valid?
    configs->>val: yo is this config any good?
option validation failed
    val->>configs: nah man it got this error
    configs-xstart: here's why your<br/>config attempt was garbage
end
val->>configs: yeah man seems legit
configs->>iuser: hot new config, fresh off the press

critical metadata might not have the address if the previous config was default, or the metadata broke
configs->>meta: what's the address of this config?

option metadata doesn't know
meta->>configs: dunno
configs->>meta: what's the next free address?
meta->>configs: here

note over configs,adapter: ConfigStorageClass should perform checks to make sure the section is empty. <br/> if it isn't, it'll alert the config user and write new metadata
end



critical write the data and check it was written correctly
    configs->>adapter: hey write this config to storage
    loop if this fails, do it again once or twice 
        adapter->>adapter :lets make a checksum
        adapter->>storage: write
        adapter->>storage: read it back to me?
        storage->>adapter: yeah there's a one and a zero and another zero...
        adapter-->>adapter: is it the same?
        adapter-->>configs: so here's how it went:
    end

    option the data just isn't reading back write, probably because the eeprom got worn
    configs->>meta: what's the next free address?
    meta->>configs: this one
    note over configs,adapter: repeat the above loop, until the next address is out of bounds

    option configs were written to a new storage location
    configs->>meta: wrote a config at this address
    opt if address is different or config is new
        meta<<->>adapter: writes the new metadata packet to storage, <br/>checks it reads back correct, wear levels if needed<br/>(same as above, but without checksum)
    end
    meta-->>configs: done
end

configs->>start: done
```
