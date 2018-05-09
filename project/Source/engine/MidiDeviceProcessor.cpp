
#include "engine/MidiDeviceProcessor.h"

namespace Element {

MidiDeviceProcessor::MidiDeviceProcessor (const bool isInput)
    : BaseProcessor(),
      inputDevice (isInput)
{ }

MidiDeviceProcessor::~MidiDeviceProcessor() noexcept { }

const String MidiDeviceProcessor::getName() const
{
    if (isInputDevice() && input != nullptr)
        return input->getName();
    if (isOutputDevice() && output != nullptr)
        return output->getName();
    return "Unknown";
}

void MidiDeviceProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    inputMessages.reset (sampleRate);
    ignoreUnused (maximumExpectedSamplesPerBlock);

    if (prepared)
        return;
    
    const int defaultIdx = inputDevice ? MidiInput::getDefaultDeviceIndex()
                                       : MidiOutput::getDefaultDeviceIndex();
    if (inputDevice)
    {
        input = MidiInput::openDevice (defaultIdx, this);
        if (input) input->start();
    }
    else
    {
        output = MidiOutput::openDevice (defaultIdx);
        if (output) output->startBackgroundThread();
    }

    prepared = true;
}

void MidiDeviceProcessor::processBlock (AudioBuffer<float>& audio, MidiBuffer& midi)
{
    const auto nframes = audio.getNumSamples();
    if (inputDevice)
    {
        midi.clear (0, nframes);
        inputMessages.removeNextBlockOfMessages (midi, nframes);
    }
    else
    {
        if (output && !midi.isEmpty())
            output->sendBlockOfMessages (
                midi, 6.0 + Time::getMillisecondCounterHiRes(), getSampleRate());
        midi.clear (0, nframes);
    }
}

void MidiDeviceProcessor::releaseResources()
{
    prepared = false;
    inputMessages.reset (getSampleRate());
    if (input)
    {
        input->stop();
        input = nullptr;
    }
    if (output)
    {
        output->stopBackgroundThread();
        output = nullptr;
    }
}

AudioProcessorEditor* MidiDeviceProcessor::createEditor()
{
    return new GenericAudioProcessorEditor (this);
}

void MidiDeviceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    ValueTree state ("state");
    state.setProperty ("inputDevice", isInputDevice(), 0)
         .setProperty ("deviceName", getName(), 0);
    if (ScopedPointer<XmlElement> xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MidiDeviceProcessor::setStateInformation (const void* data, int size)
{
    ValueTree state;
    if (ScopedPointer<XmlElement> xml = getXmlFromBinary (data, size))
        state = ValueTree::fromXml (*xml);
    if (! state.isValid())
        return;
}

void MidiDeviceProcessor::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    if (input == nullptr || input != source)
        return;
    inputMessages.addMessageToQueue (message);
}

void MidiDeviceProcessor::handlePartialSysexMessage (MidiInput* source, const uint8* messageData,
                                                     int numBytesSoFar, double timestamp)
{
    ignoreUnused (source, messageData, numBytesSoFar, timestamp);
}
                                
}