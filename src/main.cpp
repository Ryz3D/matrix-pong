#include <Arduino.h>

#define BALL_SPEED 7

int8_t pos1, pos2;
int8_t vel1, vel2;
float ball_x, ball_y;
int8_t ball_vx, ball_vy;
uint8_t matrix_buf[8];
uint16_t counter1_up, counter1_down, counter2_up, counter2_down;
bool ai_up, ai_down;
uint8_t wins1, wins2;
uint32_t lastMicros;
double deltaTime;
uint32_t hold_time;
void (*hard_reset)(void) = 0;
uint16_t boredom_counter;
uint8_t last_ball_y;

void reset_game(bool dir)
{
  pos1 = pos2 = 2;
  vel1 = vel2 = counter1_up = counter1_down = counter2_up = counter2_down = boredom_counter = 0;
  ball_x = dir ? 1 : 6;
  ball_y = last_ball_y = 3;
  ball_vx = dir ? BALL_SPEED : -BALL_SPEED;
  ball_vy = random(100, 1000) / 100.0f;
  if (random(2))
    ball_vy *= -1;
  hold_time = millis();
}

void draw()
{
  for (uint8_t y = 0; y < 8; y++)
  {
    // row
    PORTD = matrix_buf[y] << 2;
    PORTB = matrix_buf[y] >> 6;

    // col
    PORTB |= (1 << 3) | (1 << 4);
    if (y <= 5)
      PORTC = ~(1 << y);
    else
    {
      PORTC = 255;
      PORTB &= ~(1 << (y - 3));
    }
    delayMicroseconds(1000);
  }
}

void update_player(bool up, bool down, int8_t *pos, int8_t *vel, uint16_t *counter_up, uint16_t *counter_down)
{
  *counter_up = up ? *counter_up + 1 : 0;
  *counter_down = down ? *counter_down + 1 : 0;
  if (*counter_up > 20)
  {
    *pos = min(5, *pos + 1);
    *counter_up = 0;
  }
  if (*counter_down > 20)
  {
    *pos = max(0, *pos - 1);
    *counter_down = 0;
  }

  if (*counter_up > 0 && *pos < 5)
    *vel = 1;
  else if (*counter_down > 0 && *pos > 0)
    *vel = -1;
  else
    *vel = 0;
}

void check_bounce(bool dir, uint8_t pos, int8_t vel)
{
  ball_vx *= -1;
  uint8_t i_ball_y = round(ball_y);
  if (i_ball_y == pos)
    ball_vy += 2 * vel + 1;
  else if (i_ball_y == pos + 1)
    ball_vy += vel;
  else if (i_ball_y == pos + 2)
    ball_vy += 2 * vel - 1;
  else
  {
    if (dir)
      wins1++;
    else
      wins2++;
    reset_game(!dir);
  }
}

void update_ai()
{
  ai_up = ai_down = false;
  if (ball_vx < 0)
  {
    float predicted_height = ball_y + 0.07f * ball_vy * (ball_x - 1);
    int8_t target = round(predicted_height) % 8;
    if (target - pos1 - 1 > 0)
      ai_up = true;
    else if (target - pos1 - 1 < 0)
      ai_down = true;
  }
}

void setup()
{
  DDRB = 255;
  DDRC = 255;
  DDRD = 255;
  reset_game(1);
}

void loop()
{
  lastMicros = micros();

  if (millis() - hold_time > 3000)
  {
    update_ai();
    update_player(ai_up, ai_down, &pos1, &vel1, &counter1_up, &counter1_down);
  }
  if (millis() - hold_time > 1200)
    update_player(analogRead(A6) < 512, analogRead(A7) < 512, &pos2, &vel2, &counter2_up, &counter2_down);

  if (millis() - hold_time > 3000)
  {
    if (ball_x < 1)
      check_bounce(0, pos1, vel1);
    if (ball_x > 6)
      check_bounce(1, pos2, vel2);
    if (ball_y < 0 || ball_y > 7)
      ball_vy *= -1;

    ball_x += ball_vx * deltaTime;
    ball_y += ball_vy * deltaTime;

    if (round(ball_y) == last_ball_y)
      boredom_counter++;
    else
      boredom_counter = 0;
    if (boredom_counter > 250)
      reset_game(random(2));
    last_ball_y = round(ball_y);
  }

  for (uint8_t i = 0; i < 8; i++)
    matrix_buf[i] = 0;
  if (wins1 >= 8)
  {
    if (vel2 != 0)
      hard_reset();
    for (uint8_t i = 0; i < 8; i++)
      matrix_buf[i] = ((millis() / 500) % 2) << 6;
  }
  else if (wins2 >= 8)
  {
    if (vel2 != 0)
      hard_reset();
    for (uint8_t i = 0; i < 8; i++)
      matrix_buf[i] = ((millis() / 500) % 2) << 1;
  }
  else if (millis() - hold_time < 1000)
  {
    for (uint8_t i = 0; i < 8; i++)
      matrix_buf[7 - i] = (wins1 > i) << 6 | (wins2 > i) << 1;
  }
  else if (millis() - hold_time > 1200)
  {
    for (uint8_t i = 0; i < 3; i++)
      matrix_buf[pos1 + i] |= 1 << 7;
    for (uint8_t i = 0; i < 3; i++)
      matrix_buf[pos2 + i] |= 1;
    matrix_buf[round(ball_y)] |= 128 >> round(ball_x);
  }
  draw();

  deltaTime = (micros() - lastMicros) / 1000000.0;
}
