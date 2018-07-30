/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#include <besiolib/action.hpp>
#include <besiolib/crypto.h>
#include <besiolib/transaction.hpp>

#include "test_api.hpp"

#pragma pack(push, 1)
template <uint64_t ACCOUNT, uint64_t NAME>
struct test_action_action {
   static account_name get_account() {
      return account_name(ACCOUNT);
   }

   static action_name get_name() {
      return action_name(NAME);
   }

   besio::vector<char> data;

   template <typename DataStream>
   friend DataStream& operator << ( DataStream& ds, const test_action_action& a ) {
      for ( auto c : a.data )
         ds << c;
      return ds;
   }
  /*
   template <typename DataStream>
   friend DataStream& operator >> ( DataStream& ds, test_action_action& a ) {
      return ds;
   }
   */
};


template <uint64_t ACCOUNT, uint64_t NAME>
struct test_dummy_action {
   static account_name get_account() {
      return account_name(ACCOUNT);
   }

   static action_name get_name() {
      return action_name(NAME);
   }
   char a;
   unsigned long long b;
   int32_t c;

   template <typename DataStream>
   friend DataStream& operator << ( DataStream& ds, const test_dummy_action& da ) {
      ds << da.a;
      ds << da.b;
      ds << da.c;
      return ds;
   }

   template <typename DataStream>
   friend DataStream& operator >> ( DataStream& ds, test_dummy_action& da ) {
      ds >> da.a;
      ds >> da.b;
      ds >> da.c;
      return ds;
   }
};
#pragma pack(pop)

void copy_data(char* data, size_t data_len, besio::vector<char>& data_out) {
   for (unsigned int i=0; i < data_len; i++)
      data_out.push_back(data[i]);
}

void test_transaction::send_action() {
   using namespace besio;
   test_dummy_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   action act(besio::vector<permission_level>{{N(testapi), N(active)}}, test_action);
   act.send();
}

void test_transaction::send_action_empty() {
   using namespace besio;
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "assert_true")> test_action;

   action act(besio::vector<permission_level>{{N(testapi), N(active)}}, test_action);

   act.send();
}

/**
 * cause failure due to a large action payload
 */
void test_transaction::send_action_large() {
   using namespace besio;
   static char large_message[8 * 1024];
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action;
   copy_data(large_message, 8*1024, test_action.data);
   action act(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   act.send();
   besio_assert(false, "send_message_large() should've thrown an error");
}

/**
 * cause failure due recursive loop
 */
void test_transaction::send_action_recurse() {
   using namespace besio;
   char buffer[1024];
   read_action_data(buffer, 1024);

   test_action_action<N(testapi), WASM_TEST_ACTION("test_transaction", "send_action_recurse")> test_action;
   copy_data(buffer, 1024, test_action.data);
   action act(vector<permission_level>{{N(testapi), N(active)}}, test_action);

   act.send();
}

/**
 * cause failure due to inline TX failure
 */
void test_transaction::send_action_inline_fail() {
   using namespace besio;
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "assert_false")> test_action;

   action act(vector<permission_level>{{N(testapi), N(active)}}, test_action);

   act.send();
}

void test_transaction::test_tapos_block_prefix() {
   using namespace besio;
   int tbp;
   read_action_data( (char*)&tbp, sizeof(int) );
   besio_assert( tbp == tapos_block_prefix(), "tapos_block_prefix does not match" );
}

void test_transaction::test_tapos_block_num() {
   using namespace besio;
   int tbn;
   read_action_data( (char*)&tbn, sizeof(int) );
   besio_assert( tbn == tapos_block_num(), "tapos_block_num does not match" );
}


void test_transaction::test_read_transaction() {
   using namespace besio;
   checksum256 h;
   auto size = transaction_size();
   char buf[size];
   uint32_t read = read_transaction( buf, size );
   besio_assert( size == read, "read_transaction failed");
   sha256(buf, read, &h);
   printhex( &h, sizeof(h) );
}

void test_transaction::test_transaction_size() {
   using namespace besio;
   uint32_t trans_size = 0;
   read_action_data( (char*)&trans_size, sizeof(uint32_t) );
   print( "size: ", transaction_size() );
   besio_assert( trans_size == transaction_size(), "transaction size does not match" );
}

void test_transaction::send_transaction(uint64_t receiver, uint64_t, uint64_t) {
   using namespace besio;
   dummy_action payload = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};

   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action;
   copy_data((char*)&payload, sizeof(dummy_action), test_action.data);

   auto trx = transaction();
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0, receiver);
}

void test_transaction::send_action_sender(uint64_t receiver, uint64_t, uint64_t) {
   using namespace besio;
   account_name cur_send;
   read_action_data( &cur_send, sizeof(account_name) );
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "test_current_sender")> test_action;
   copy_data((char*)&cur_send, sizeof(account_name), test_action.data);

   auto trx = transaction();
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0, receiver);
}

void test_transaction::send_transaction_empty(uint64_t receiver, uint64_t, uint64_t) {
   using namespace besio;
   auto trx = transaction();
   trx.send(0, receiver);

   besio_assert(false, "send_transaction_empty() should've thrown an error");
}

void test_transaction::send_transaction_trigger_error_handler(uint64_t receiver, uint64_t, uint64_t) {
   using namespace besio;
   auto trx = transaction();
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "assert_false")> test_action;
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0, receiver);
}

void test_transaction::assert_false_error_handler(const besio::transaction& dtrx) {
   besio_assert(dtrx.actions.size() == 1, "transaction should only have one action");
   besio_assert(dtrx.actions[0].account == N(testapi), "transaction has wrong code");
   besio_assert(dtrx.actions[0].name == WASM_TEST_ACTION("test_action", "assert_false"), "transaction has wrong name");
   besio_assert(dtrx.actions[0].authorization.size() == 1, "action should only have one authorization");
   besio_assert(dtrx.actions[0].authorization[0].actor == N(testapi), "action's authorization has wrong actor");
   besio_assert(dtrx.actions[0].authorization[0].permission == N(active), "action's authorization has wrong permission");
}

/**
 * cause failure due to a large transaction size
 */
void test_transaction::send_transaction_large(uint64_t receiver, uint64_t, uint64_t) {
   using namespace besio;
   auto trx = transaction();
   for (int i = 0; i < 32; i ++) {
      char large_message[1024];
      test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action;
      copy_data(large_message, 1024, test_action.data);
      trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   }

   trx.send(0, receiver);

   besio_assert(false, "send_transaction_large() should've thrown an error");
}

/**
 * deferred transaction
 */
void test_transaction::deferred_print() {
   besio::print("deferred executed\n");
}

void test_transaction::send_deferred_transaction(uint64_t receiver, uint64_t, uint64_t) {
   using namespace besio;
   auto trx = transaction();
   test_action_action<N(testapi), WASM_TEST_ACTION("test_transaction", "deferred_print")> test_action;
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.delay_sec = 2;
   trx.send( 0xffffffffffffffff, receiver );
}

void test_transaction::send_deferred_transaction_replace(uint64_t receiver, uint64_t, uint64_t) {
   using namespace besio;
   auto trx = transaction();
   test_action_action<N(testapi), WASM_TEST_ACTION("test_transaction", "deferred_print")> test_action;
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.delay_sec = 2;
   trx.send( 0xffffffffffffffff, receiver, true );
}

void test_transaction::send_deferred_tx_with_dtt_action() {
   using namespace besio;
   dtt_action dtt_act;
   read_action_data(&dtt_act, action_data_size());

   action deferred_act;
   deferred_act.account = dtt_act.deferred_account;
   deferred_act.name = dtt_act.deferred_action;
   deferred_act.authorization = vector<permission_level>{{N(testapi), dtt_act.permission_name}};

   auto trx = transaction();
   trx.actions.emplace_back(deferred_act);
   trx.delay_sec = dtt_act.delay_sec;
   trx.send( 0xffffffffffffffff, dtt_act.payer, true );
}


void test_transaction::cancel_deferred_transaction_success() {
   using namespace besio;
   auto r = cancel_deferred( 0xffffffffffffffff ); //use the same id (0) as in send_deferred_transaction
   besio_assert( (bool)r, "transaction was not found" );
}

void test_transaction::cancel_deferred_transaction_not_found() {
   using namespace besio;
   auto r = cancel_deferred( 0xffffffffffffffff ); //use the same id (0) as in send_deferred_transaction
   besio_assert( !r, "transaction was canceled, whild should not be found" );
}

void test_transaction::send_cf_action() {
   using namespace besio;
   test_action_action<N(dummy), N(event1)> cfa;
   action act(cfa);
   act.send_context_free();
}

void test_transaction::send_cf_action_fail() {
   using namespace besio;
   test_action_action<N(dummy), N(event1)> cfa;
   action act(vector<permission_level>{{N(dummy), N(active)}}, cfa);
   act.send_context_free();
   besio_assert(false, "send_cfa_action_fail() should've thrown an error");
}

void test_transaction::stateful_api() {
   char buf[4] = {1};
   db_store_i64(N(test_transaction), N(table), N(test_transaction), 0, buf, 4);
}

void test_transaction::context_free_api() {
   char buf[128] = {0};
   get_context_free_data(0, buf, sizeof(buf));
}

extern "C" { int is_feature_active(int64_t); }
void test_transaction::new_feature() {
   besio_assert(false == is_feature_active((int64_t)N(newfeature)), "we should not have new features unless hardfork");
}

void test_transaction::active_new_feature() {
   activate_feature((int64_t)N(newfeature));
}
