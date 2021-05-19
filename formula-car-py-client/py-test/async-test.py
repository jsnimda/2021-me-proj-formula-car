import asyncio
import time


async def tt():
    await asyncio.sleep(2)
    print('wake from async')

# loop = asyncio.get_event_loop()
# task = asyncio.create_task(tt())
asyncio.run(tt())

for i in range(5):
    print(i)
    time.sleep(1)
