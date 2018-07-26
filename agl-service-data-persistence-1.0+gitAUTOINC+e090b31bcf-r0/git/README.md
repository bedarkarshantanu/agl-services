
# Database Binding
This binding provide a database API with key/value semantics.
The backend is currently a Berkeley DB.

## Verbs
* **insert**:
	This verb insert a key/value pair in the database.
	If the key already exist, the verb fails.
	
* **update**:
	This verb update an existing record.
	If the key doesn't exist, the verb fails.
	
* **delete**:
	This verb remove an existing key/value pair from the database.
	If no matching record is found, the verb fails.

* **read**:
	This verb get the value associated with the specified key.
	If no matching record is found, the verb fails.

## Arguments
* The **read** and **delete** verbs need only a **key** to work:
```
{
	"key": "mykey"
}
```

* The **insert** and **update** verbs need a **key** and a **value** to work:
```
{
	"key": "mykey",
	"value": "my value"
}
```
The **value** can be any valid json.
