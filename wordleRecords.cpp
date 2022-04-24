#include <Arduino.h>
#include <regex.h>
#include "wordleRecords.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

WordleRecords::WordleRecords()
{
  records = NULL;
  records_size  = 0;
  num_records  = 0;
  latest_record = -1;
  next_fetched_record = -1;
  new_record_avail = false;
}

WordleRecords::~WordleRecords()
{
  free(records);
  records = NULL;
}

bool WordleRecords::add(char *data, uint16_t len)
{

  enum message_states {BEGIN, IN_FROM, BETWEEN, IN_BODY, FINISHED};

  int i=0;
  char *from = NULL;
  char *body = NULL;
  int body_len = 0;
  enum message_states state = BEGIN;

  //Serial.printf("Got %d bytes of world data:\r\n", len);
  //Serial.println(data);

  while (i < len) {
    if (state == IN_BODY) {
      body_len++;
    }

    //Serial.printf("i=%d (0x%x) (body_len=%d)\r\n", i, data[i], body_len);
    if (data[i] == '<') {
      switch(state) {
        case BEGIN:
          from=&(data[i+1]);
          state = IN_FROM;
          //Serial.printf(" i=%d (%c): BEGIN->IN_FROM\r\n", i, data[i]);
          break;
        case BETWEEN:
          body=&(data[i+1]);
          state = IN_BODY;
          //Serial.printf(" i=%d (%c): BETWEEN->IN_BODY\r\n", i, data[i]);
          break;
        default:
          Serial.printf("  Invalid char for state %d\r\n", state);
          return false;
      }
    } else if (data[i] == '>') {
      switch(state) {
        case IN_FROM:
          data[i] = '\0';
          state = BETWEEN;
          //Serial.printf(" i=%d (%c): IN_FROM->BETWEEN\r\n", i, data[i]);
          break;
        case IN_BODY:
          data[i] = '\0';
          state = FINISHED;
          //Serial.printf(" i=%d (%c): IN_BODY->FINISHED\r\n", i, data[i]);
          break;
        default:
          Serial.printf("  Invalid char for state %d\r\n", state);
          return false;
      }
    }
    i++;
  }

  if (state == FINISHED) {
    //Serial.printf("parsed from: %s\r\n", from);
    record result = parseBody(body, body_len);
    if (result.score) {
      strncpy(result.user, from, MAX_NAME_LEN);
      //printRecord(result);
      return add(result);
    }
    else Serial.printf("parsed body failed\r\n");
  }

  Serial.println("FAILED TO  PARSE INPUT");
  return false;
}

bool WordleRecords::add(record r)
{
    if (records_size < MAX_RECORDS)
    {
      /* Allocate a new record in the array */
      records = (record *)reallocarray(records, records_size + 1, sizeof(record));
      if (!records)
      {
        Serial.println("Memory allocation failure");
        return false;
      }
      records_size++;
    }

    /* Copy the record */
    if (++latest_record >= records_size)
    {
      Serial.println("Record wraparound");
      latest_record = 0;
    }
    records[latest_record] = r;

    if (num_records < records_size)
    {
      num_records += 1;
    } else {
      Serial.println("Record overflow; dropping oldest");
    }

    next_fetched_record = latest_record;
    new_record_avail = true;
    Serial.printf("Added record-%d (%d valid) (size: %d)\r\n", \
                  latest_record, num_records, records_size);
    return true;
}

void WordleRecords::reset()
{
  num_records  = 0;
  latest_record = -1;
  next_fetched_record = -1;
  new_record_avail = false;
}

bool WordleRecords::isNewRecordAvail()
{
  bool ret = new_record_avail;
  new_record_avail = false;
  return ret;
}

/* Should refresh if there are multiple records available */
bool WordleRecords::shouldRefresh()
{
  return (num_records > 1);
}

record WordleRecords::get()
{
  record val;
  /* Return a '0' record if we don't have any */
  if (latest_record < 0)
  {
    return {0};
  }

  val = records[next_fetched_record--];
  if (next_fetched_record < 0)
  {
    next_fetched_record = num_records - 1;        
  }
  return val;
}

void WordleRecords::printRecord(record result)
{
  int i,j;

  Serial.printf("User: %s\r\n", result.user);
  Serial.printf("Game #: %d\r\n", result.gameno);
  Serial.printf("Score: %d\r\n", result.score);
  Serial.printf("Hard: %d\r\n", result.hard);
  Serial.println("Results:");
  for(i=0; i<MIN(result.score,6); i++) {
    for(j=0; j<WORD_LEN; j++) {
      Serial.printf(" %c ", state_to_char(result.results[i][j]));
    }
    Serial.println("");
  }
}

/*
 * Intepret a char string as a UTF-8 string at offset.  Increment offset
 * as appropriate.  Returns the next UTF-8 code point as a uin_32_t
 */
uint32_t WordleRecords::get_next_utf8(char *string, int *offset) {
  /* UTF-8 first byte meaning
    00 to 7F hex (0 to 127): first and only byte of a sequence.
    80 to BF hex (128 to 191): continuing byte in a multi-byte sequence.
    C2 to DF hex (194 to 223): first byte of a two-byte sequence.
    E0 to EF hex (224 to 239): first byte of a three-byte sequence.
    F0 to FF hex (240 to 255): first byte of a four-byte sequence.
  */
  uint32_t result = 0;

  uint8_t b1 = string[*offset];

  if (b1 <= 0x7F) {
    result = b1;
    *offset += 1;
  } else if ((b1 >= 0xC2) && (b1 <= 0xDF)) {
    result = (b1 << 8) | (string[(*offset)+1]);
    *offset = *offset + 2;
  } else if ((b1 >= 0xe0) && (b1 <= 0xEF)) {
    result = (b1 << 16) | ((string[(*offset)+1]) << 8) | (string[(*offset)+2]);
    *offset = *offset + 3;
  } else if (b1 >= 0xf0) {
    result = (b1 << 24) | (string[(*offset)+1] << 16) | (string[(*offset)+2] << 8) | string[(*offset)+3];
    *offset = *offset + 4;
  }

  return result;
}

char WordleRecords::state_to_char(state s)
{
  switch(s) {
    case UNUSED:
      return (' ');
    case CORRECT:
      return ('G');
    case CLOSE:
      return ('Y');
    case WRONG:
      return ('W');
    default:
      return ('?');
  }
}

record WordleRecords::parseBody(char *data, int len)
{
  record result = {0};
  uint32_t c;
  int i = 0;
  int guess = 0;
  int g_off = 0;

  #define NUM_MATCHES 5
  static regex_t regexCompiled = {0};
  regmatch_t groups[NUM_MATCHES+1];

  // Parse: 'Wordle 222 5/6*'
  /* Just compile once */
  if (!regexCompiled.re_magic) {
    if (regcomp(&regexCompiled, "Wordle[ ]+([0-9]+)[ ]+([0-9xX)/([0-9])(.)", REG_EXTENDED))
    {
      Serial.print("Unable to compile regex");
      return result;
    };
  };

  if (regexec(&regexCompiled, data, NUM_MATCHES, groups, 0) == 0)
  {
#ifdef DEBUG
    Serial.println("Got a match");

    Serial.printf("group-0 start: %d\r\n", groups[0].rm_so);
    Serial.printf("group-0 end: %d\r\n", groups[0].rm_eo);

    Serial.printf("group-1 start: %d\r\n", groups[1].rm_so);
    Serial.printf("group-1 end: %d\r\n", groups[1].rm_eo);

    Serial.printf("group-2 start: %d\r\n", groups[2].rm_so);
    Serial.printf("group-2 end: %d\r\n", groups[2].rm_eo);

    Serial.printf("group-3 start: %d\r\n", groups[3].rm_so);
    Serial.printf("group-3 end: %d\r\n", groups[3].rm_eo);

    Serial.printf("group-4 start: %d\r\n", groups[4].rm_so);
    Serial.printf("group-4 end: %d\r\n", groups[4].rm_eo);
#endif
    // work from back to front since we're modifying string
    if (groups[4].rm_so < len)
    {
      if (data[groups[4].rm_so] == '*')
        result.hard = true;
      data[groups[4].rm_eo] = '\0';
      //Serial.printf("parsed hard flag (%s) as: %d\r\n", &(data[groups[4].rm_so]), result.hard);
    }

    if (groups[3].rm_so < len)
      i = MAX(i, groups[3].rm_eo);

    if (groups[2].rm_so < len)
    {
      i = MAX(i, groups[2].rm_eo);
      data[groups[2].rm_eo] = '\0';
      if ((data[groups[2].rm_so] == 'X') || (data[groups[2].rm_so] == 'x'))
      {
        result.score = 255;
      } else {
        result.score = atoi(&(data[groups[2].rm_so]));
      }
      //Serial.printf("parsed score (%s) as: %d\r\n", &(data[groups[2].rm_so]), result.score);
    }

    if (groups[1].rm_so < len)
    {
      data[groups[1].rm_eo] = '\0';
      result.gameno = atoi(&(data[groups[1].rm_so]));
      //Serial.printf("parsed gameno (%s) as: %d\r\n", &(data[groups[1].rm_so]), result.gameno);
    }
  } else {
    Serial.println("No regex match");
    return result;
  }

  // Parse UTF-8 colored squares
  while (i < len) {
    c = get_next_utf8(data, &i);
    if (c < 0x7f) {
      //Serial.printf("  i: %d  c: '%c' (0x%x)\r\n", i, c, c);
    } else if (c == 0xF09F9FA8) {
      //Serial.printf("  i: %d  guess[%d][%d] = [%c] (0x%x)\r\n", i, guess, g_off, state_to_char(CLOSE), c);
      result.results[guess][g_off++] = CLOSE;
    } else if (c == 0xF09F9FA9)  {
      //Serial.printf("  i: %d  guess[%d][%d] = [%c] (0x%x)\r\n", i, guess, g_off, state_to_char(CORRECT), c);
      result.results[guess][g_off++] = CORRECT;
    } else if ((c == 0xE2AC9B) || (c == 0xE2AC9C)) {
      //Serial.printf("  i: %d  guess[%d][%d] = [%c] (0x%x)\r\n", i, guess, g_off, state_to_char(WRONG), c);
      result.results[guess][g_off++] = WRONG;
    } else if ((c == ' ') || (c == '\n')) {
      //Serial.printf("  i: %d  c: [ ] (0x%x)\r\n", i, c);
      guess++;
      g_off = 0;
    } else {
      //Serial.printf("  i: %d  c: ??? (0x%x)\r\n", i, c);
    }

    if (g_off == WORD_LEN)
    {
      guess++;
      g_off = 0;
    }
    if (guess == GUESSES)
      break;
  }

  return result;
}
