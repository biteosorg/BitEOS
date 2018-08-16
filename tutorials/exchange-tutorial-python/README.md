The following steps must be taken for the example script to work.

0. Create wallet
0. Create account for besio.token
0. Create account for scott
0. Create account for exchange
0. Set token contract on besio.token
0. Create BES token
0. Issue initial tokens to scott

**Note**:
Deleting the `transactions.txt` file will prevent replay from working.


### Create wallet
`clbes wallet create`

### Create account steps
`clbes create key`

`clbes create key`

`clbes wallet import  --private-key <private key from step 1>`

`clbes wallet import  --private-key <private key from step 2>`

`clbes create account besio <account_name> <public key from step 1> <public key from step 2>`

### Set contract steps
`clbes set contract besio.token /contracts/besio.token -p besio.token@active`

### Create BES token steps
`clbes push action besio.token create '{"issuer": "besio.token", "maximum_supply": "100000.0000 BES", "can_freeze": 1, "can_recall": 1, "can_whitelist": 1}' -p besio.token@active`

### Issue token steps
`clbes push action besio.token issue '{"to": "scott", "quantity": "900.0000 BES", "memo": "testing"}' -p besio.token@active`
