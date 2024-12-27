import whisper
import os
import sys
os.environ['HF_ENDPOINT'] = 'https://hf-mirror.com'
from pyannote.audio import Pipeline

hf_token = "example"

# Step 1: 语音转文本
audio_file = sys.argv[1]
whisper_model = whisper.load_model("base")
result = whisper_model.transcribe(audio_file)

# Step 2: 说话人分离
pipeline = Pipeline.from_pretrained("pyannote/speaker-diarization", use_auth_token=hf_token)
diarization = pipeline(audio_file)

# Step 3: 合并同一发言者的段落
segments = result["segments"]
speaker_turns = list(diarization.itertracks(yield_label=True))

merged_results = []
current_speaker = None
current_start = None
current_end = None
current_text = []

for segment in segments:
    start, end, text = segment["start"], segment["end"], segment["text"]

    # 找到此段对应的说话人
    for turn, _, speaker in speaker_turns:
        if turn.start <= start <= turn.end:
            segment_speaker = speaker
            break
    else:
        segment_speaker = "Unknown"

    # 合并逻辑
    if segment_speaker == current_speaker and current_end and (start - current_end <= 4):
        # 同一个人，并且时间间隔在1秒以内，合并
        current_end = end
        current_text.append(text)
    else:
        # 不同的人或时间间隔过大，保存上一个段落，开始新段落
        if current_speaker is not None:
            merged_results.append({
                "speaker": current_speaker,
                "start": current_start,
                "end": current_end,
                "text": " ".join(current_text),
            })
        # 更新当前段落
        current_speaker = segment_speaker
        current_start = start
        current_end = end
        current_text = [text]

# 保存最后一个段落
if current_speaker is not None:
    merged_results.append({
        "speaker": current_speaker,
        "start": current_start,
        "end": current_end,
        "text": " ".join(current_text),
    })

# Step 4: 打印结果
output_file = "output.txt"

with open(output_file, "w", encoding="utf-8") as f:
    for result in merged_results:
        line = f"[{result['start']:.2f}-{result['end']:.2f}] Speaker {result['speaker']}: {result['text']}\n"
        f.write(line)

print(f"合并后的结果已保存到 {output_file}")

