#ifndef __WORDLE_H
#define __WORDLE_H

#define WORD_LEN         5
#define GUESSES          6
#define MAX_NAME_LEN    24   // Max user name len (including /0)
#define MAX_RECORDS      5   // Keep under 128 due to int8_t used for latest_record

typedef enum state {UNUSED, CORRECT, CLOSE, WRONG} state;

/* No pointers, depends on shallow copy */
typedef struct record {
  char user[MAX_NAME_LEN];
  unsigned short score;
  unsigned int gameno;
  bool hard;
  state results[GUESSES][WORD_LEN];
} record;

class WordleRecords
{
  public:
    WordleRecords();           // List of records
    ~WordleRecords();
    bool add(char *data, uint16_t len); // Add record from a wordle string
    bool add(record r);        // Add a record
    void reset();              // Reset the list to empty
    bool isNewRecordAvail();   // new record since last call?
    bool shouldRefresh();      // Is it worth refreshing (are there multiple records?)
    record get();              // returns the next record that should be shown
    void printRecord(record r);
  private:
    record *records  = NULL;
    uint8_t records_size  = 0; // Number of records malloc'ed
    uint8_t num_records  = 0;  // Number of valid records currently held
    int latest_record = -1;    // index to latest valid record (or -1 if none)
    int8_t next_fetched_record = 0; // index to the next record to be fetched (displayed)
    bool new_record_avail = false;  // Flag to notify display to restart at latest
    record parseBody(char *data, int len);
    char state_to_char(state s);
    uint32_t get_next_utf8(char *string, int *offset);
};

#endif // __WORDLE_H
