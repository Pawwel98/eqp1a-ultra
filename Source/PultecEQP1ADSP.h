#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <atomic>

namespace PultecDSP
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 6.28318530717958647692;

inline double dbToLin(double db) noexcept
{
    return std::pow(10.0, db * 0.05);
}

inline double linToDb(double lin) noexcept
{
    return 20.0 * std::log10(std::max(lin, 1.0e-12));
}

inline double fastTanh(double x) noexcept
{
    if (x < -4.5) return -1.0;
    if (x > 4.5)  return 1.0;
    const double x2 = x * x;
    return x * (27.0 + x2) / (27.0 + 9.0 * x2 + 0.12 * x2 * x2);
}

//==============================================================================
// 1. Topology-Preserving Transform (TPT / Zero-Delay Feedback) Filter
//    Symmetrical wide-skirt analog passive bell & shelf parameterization
//==============================================================================
struct TPTFilter
{
    double ic1eq = 0.0;
    double ic2eq = 0.0;

    double g = 0.0, k = 1.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
    double m0 = 1.0, m1 = 0.0, m2 = 0.0;

    void reset() noexcept
    {
        ic1eq = 0.0;
        ic2eq = 0.0;
    }

    void setLowShelf(double sampleRate, double cutoffHz, double Q, double gainDb) noexcept
    {
        cutoffHz = std::clamp(cutoffHz, 12.0, sampleRate * 0.45);
        Q = std::clamp(Q, 0.25, 4.0);
        const double A = std::pow(10.0, gainDb / 40.0);
        g = std::tan(kPi * cutoffHz / sampleRate) / std::sqrt(A);
        k = 1.0 / Q;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1.0;
        m1 = k * (A - 1.0);
        m2 = (A * A - 1.0);
    }

    void setHighShelf(double sampleRate, double cutoffHz, double Q, double gainDb) noexcept
    {
        cutoffHz = std::clamp(cutoffHz, 80.0, sampleRate * 0.45);
        Q = std::clamp(Q, 0.25, 4.0);
        const double A = std::pow(10.0, gainDb / 40.0);
        g = std::tan(kPi * cutoffHz / sampleRate) * std::sqrt(A);
        k = 1.0 / Q;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = A * A;
        m1 = k * (1.0 - A) * A;
        m2 = (1.0 - A * A);
    }

    void setBell(double sampleRate, double centerHz, double Q, double gainDb) noexcept
    {
        centerHz = std::clamp(centerHz, 15.0, sampleRate * 0.45);
        Q = std::clamp(Q, 0.15, 6.0);
        const double A = std::pow(10.0, gainDb / 40.0);
        g = std::tan(kPi * centerHz / sampleRate);
        k = 1.0 / std::max(Q * std::sqrt(A), 0.10);
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1.0;
        m1 = k * (A * A - 1.0);
        m2 = 0.0;
    }

    inline double processSample(double v0) noexcept
    {
        const double v3 = v0 - ic2eq;
        const double v1 = a1 * ic1eq + a2 * v3;
        const double v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;
        return m0 * v0 + m1 * v1 + m2 * v2;
    }

    double getMagnitudeDbAt(double freqHz, double sampleRate) const noexcept
    {
        if (freqHz <= 1.0 || freqHz >= sampleRate * 0.49) return 0.0;
        const double w = std::tan(kPi * freqHz / sampleRate);
        const double s = w / std::max(g, 1.0e-9);
        const double s2 = s * s;
        const double denR = 1.0 - s2;
        const double denI = k * s;
        const double numR = m0 * (1.0 - s2) + m2;
        const double numI = (m0 * k + m1) * s;
        const double mag2 = (numR * numR + numI * numI) / std::max(denR * denR + denI * denI, 1.0e-18);
        return 10.0 * std::log10(std::max(mag2, 1.0e-12));
    }
};

//==============================================================================
// 2. Authentic Pultec EQP-1A3 Coupled Passive LCR Network (Matched to Hardware & UAD/Waves)
//    - LOW FREQUENCY (CPS): 4 authentic taps (20, 30, 60, 100 CPS)
//    - HIGH FREQUENCY (KCS): 7 authentic taps (3, 4, 5, 8, 10, 12, 16 KCS)
//    - ATTEN SEL (KCS): 3 authentic taps (5, 10, 20 KCS)
//==============================================================================
class CoupledPassivePultecNetwork
{
public:
    static constexpr std::array<double, 4> kLowFreqsHz  = { 20.0, 30.0, 60.0, 100.0 };
    static constexpr std::array<double, 7> kHighBoostHz = { 3000.0, 4000.0, 5000.0, 8000.0, 10000.0, 12000.0, 16000.0 };
    static constexpr std::array<double, 3> kHighAttenHz = { 5000.0, 10000.0, 20000.0 };

    // Real Pultec EQP-1A wide passive RC low-boost shelf midpoint frequencies:
    static constexpr std::array<double, 4> kLowBoostTurnoverHz = { 115.0, 165.0, 285.0, 440.0 };
    // Low Atten turnover frequencies (when Low Boost is 0):
    static constexpr std::array<double, 4> kLowAttenTurnoverHz = { 185.0, 265.0, 450.0, 680.0 };
    // Pultec Trick Low-Mid Dip frequencies when both Low Boost and Low Atten are engaged:
    static constexpr std::array<double, 4> kLowTrickDipHz      = { 190.0, 275.0, 470.0, 720.0 };

    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(44100.0, newSampleRate);
        eddyEnvCoeff = std::exp(-1.0 / (0.005 * sampleRate));
        reset();
        updateCoefficients();
    }

    void reset() noexcept
    {
        lowBoostShelf.reset();
        lowSubBodyBell.reset();
        lowAttenShelf.reset();
        lowMutualDipBell.reset();
        highBoostInductorBell.reset();
        highSilkDeHarshShelf.reset();
        highAttenShelf.reset();
        hfEddyEnv = 0.0;
    }

    void setParameters(int lowFreqIdx,
                       double lowBoostDial0to10,
                       double lowAttenDial0to10,
                       int highBoostFreqIdx,
                       double highBoostDial0to10,
                       double bandwidthDial0to10,
                       int highAttenFreqIdx,
                       double highAttenDial0to10) noexcept
    {
        const int clampedLowIdx = std::clamp(lowFreqIdx, 0, (int)kLowFreqsHz.size() - 1);
        const int clampedHBIdx  = std::clamp(highBoostFreqIdx, 0, (int)kHighBoostHz.size() - 1);
        const int clampedHAIdx  = std::clamp(highAttenFreqIdx, 0, (int)kHighAttenHz.size() - 1);

        const double lb = std::clamp(lowBoostDial0to10, 0.0, 10.0);
        const double la = std::clamp(lowAttenDial0to10, 0.0, 10.0);
        const double hb = std::clamp(highBoostDial0to10, 0.0, 10.0);
        const double bw = std::clamp(bandwidthDial0to10, 0.0, 10.0);
        const double ha = std::clamp(highAttenDial0to10, 0.0, 10.0);

        if (clampedLowIdx == curLowIdx && clampedHBIdx == curHBIdx && clampedHAIdx == curHAIdx &&
            std::abs(lb - curLowBoost) < 1e-4 && std::abs(la - curLowAtten) < 1e-4 &&
            std::abs(hb - curHighBoost) < 1e-4 && std::abs(bw - curBandwidth) < 1e-4 &&
            std::abs(ha - curHighAtten) < 1e-4)
        {
            return;
        }

        curLowIdx    = clampedLowIdx;
        curHBIdx     = clampedHBIdx;
        curHAIdx     = clampedHAIdx;
        curLowBoost  = lb;
        curLowAtten  = la;
        curHighBoost = hb;
        curBandwidth = bw;
        curHighAtten = ha;

        updateCoefficients();
    }

    void updateCoefficients() noexcept
    {
        // =====================================================================
        // 1. LOW FREQUENCY SECTION (Coupled 10k Boost + 100k Atten Passive Bridge)
        // =====================================================================
        const double lbNorm = curLowBoost * 0.1;
        const double laNorm = curLowAtten * 0.1;

        const double lbTaper = (lbNorm > 0.0) ? std::pow(lbNorm, 0.72) : 0.0;
        const double laTaper = (laNorm > 0.0) ? std::pow(laNorm, 1.08) : 0.0;

        const double boostShuntFactor = 1.0 - 0.72 * std::pow(lbTaper, 0.60);

        const double lowBoostDb = 14.5 * lbTaper;
        const double lowAttenShelfDb = -17.5 * laTaper * boostShuntFactor;

        const double fBoostCorner = kLowBoostTurnoverHz[(size_t)curLowIdx] * (1.05 - 0.12 * lbTaper);
        const double fAttenCorner = kLowAttenTurnoverHz[(size_t)curLowIdx] * (1.0 + 0.35 * lbTaper);

        lowBoostShelf.setLowShelf(sampleRate, fBoostCorner, 0.42, lowBoostDb);

        const double fSub = kLowFreqsHz[(size_t)curLowIdx] * 1.75;
        lowSubBodyBell.setBell(sampleRate, fSub, 0.52, 2.2 * lbTaper * (1.0 - 0.22 * laTaper));

        lowAttenShelf.setLowShelf(sampleRate, fAttenCorner, 0.46, lowAttenShelfDb);

        const double mutualLow = std::sqrt(lbTaper * laTaper);
        const double dipFreqHz = kLowTrickDipHz[(size_t)curLowIdx];
        const double dipGainDb = -6.8 * laTaper * std::pow(lbTaper, 0.42);
        lowMutualDipBell.setBell(sampleRate, dipFreqHz, 0.72 + 0.25 * mutualLow, dipGainDb);

        // =====================================================================
        // 2. HIGH FREQUENCY RESONANT INDUCTOR BOOST (Silky, Warm, Never Sharp)
        // =====================================================================
        const double fHighBoost = kHighBoostHz[(size_t)curHBIdx];
        const double hbNorm     = curHighBoost * 0.1;
        const double bwNorm     = curBandwidth * 0.1; // 0 = Sharp, 1 = Broad

        const double hbTaper = (hbNorm > 0.0) ? std::pow(hbNorm, 1.08) : 0.0;

        const double maxHighGainDb = 15.5 - 7.0 * std::pow(bwNorm, 0.78);
        const double highBoostDb   = maxHighGainDb * hbTaper;

        // Broad, musical passive inductor Q (Sharp=0 -> Q 0.78, Mid=5 -> Q 0.42, Broad=10 -> Q 0.26)
        const double effectiveQ = 0.78 * std::pow(0.33, bwNorm);

        highBoostInductorBell.setBell(sampleRate, fHighBoost, effectiveQ, highBoostDb);

        const double cushionFreq = std::clamp(fHighBoost * 0.55, 2800.0, 5200.0);
        const double cushionDb   = -1.25 * hbTaper * (1.0 - 0.35 * bwNorm);
        highSilkDeHarshShelf.setBell(sampleRate, cushionFreq, 0.65, cushionDb);

        inductorEddyAmount = hbTaper * 0.42;

        // =====================================================================
        // 3. HIGH FREQUENCY ATTENUATION SHELF (ATTEN SEL: 5, 10, 20 KCS)
        // =====================================================================
        const double fHighAtten = kHighAttenHz[(size_t)curHAIdx];
        const double haNorm     = curHighAtten * 0.1;
        const double haTaper    = (haNorm > 0.0) ? std::pow(haNorm, 0.92) : 0.0;
        const double highAttenDb = -16.0 * haTaper;

        const double fHighAttenCorner = fHighAtten * (0.62 - 0.08 * haTaper);
        highAttenShelf.setHighShelf(sampleRate, fHighAttenCorner, 0.45, highAttenDb);
    }

    inline double processSample(double x) noexcept
    {
        double y = lowBoostShelf.processSample(x);
        y = lowSubBodyBell.processSample(y);
        y = lowAttenShelf.processSample(y);
        y = lowMutualDipBell.processSample(y);

        const double beforeHB = y;
        double afterHB = highBoostInductorBell.processSample(beforeHB);
        if (inductorEddyAmount > 0.001)
        {
            const double hfDelta = afterHB - beforeHB;
            hfEddyEnv = eddyEnvCoeff * hfEddyEnv + (1.0 - eddyEnvCoeff) * std::abs(hfDelta);
            const double softDamp = 1.0 / (1.0 + inductorEddyAmount * hfEddyEnv * 2.2);
            afterHB = beforeHB + hfDelta * softDamp;
        }
        y = highSilkDeHarshShelf.processSample(afterHB);

        y = highAttenShelf.processSample(y);
        return y;
    }

    double getMagnitudeDbAtFrequency(double freqHz) const noexcept
    {
        return lowBoostShelf.getMagnitudeDbAt(freqHz, sampleRate)
             + lowSubBodyBell.getMagnitudeDbAt(freqHz, sampleRate)
             + lowAttenShelf.getMagnitudeDbAt(freqHz, sampleRate)
             + lowMutualDipBell.getMagnitudeDbAt(freqHz, sampleRate)
             + highBoostInductorBell.getMagnitudeDbAt(freqHz, sampleRate)
             + highSilkDeHarshShelf.getMagnitudeDbAt(freqHz, sampleRate)
             + highAttenShelf.getMagnitudeDbAt(freqHz, sampleRate);
    }

private:
    double sampleRate = 48000.0;
    int curLowIdx = -1, curHBIdx = -1, curHAIdx = -1;
    double curLowBoost = -1.0, curLowAtten = -1.0;
    double curHighBoost = -1.0, curBandwidth = -1.0, curHighAtten = -1.0;

    double inductorEddyAmount = 0.0;
    double hfEddyEnv = 0.0;
    double eddyEnvCoeff = 0.995;

    TPTFilter lowBoostShelf;
    TPTFilter lowSubBodyBell;
    TPTFilter lowAttenShelf;
    TPTFilter lowMutualDipBell;
    TPTFilter highBoostInductorBell;
    TPTFilter highSilkDeHarshShelf;
    TPTFilter highAttenShelf;
};

//==============================================================================
// 3. Three-Transformer Magnetic Hysteresis & Warm Analog Contour Engine
//    (Triad HS-56 Input + Triad HS-29 Interstage + Peerless S-217-D Output)
//==============================================================================
class TriTransformerHysteresisEngine
{
public:
    void prepare(double newSampleRate, double channelToleranceOffset = 0.0) noexcept
    {
        sampleRate = std::max(44100.0, newSampleRate);
        tol = channelToleranceOffset;

        const double fFlux = 52.0 * (1.0 + 0.025 * tol);
        fluxPole  = std::exp(-kTwoPi * fFlux / sampleRate);
        fluxScale = (1.0 - fluxPole);

        dcBlockPole = std::exp(-kTwoPi * 6.5 / sampleRate);

        ironBassWeight.setBell(sampleRate, 55.0 * (1.0 + 0.02 * tol), 0.50, 1.25);
        hs29SilkRollOff.setHighShelf(sampleRate, 13800.0 * (1.0 + 0.015 * tol), 0.48, -1.20);

        reset();
    }

    void reset() noexcept
    {
        fluxHS56 = 0.0;
        prevMagHS56 = 0.0;
        fluxPeerless = 0.0;
        dcStateIn = 0.0;
        dcStateOut = 0.0;
        ironBassWeight.reset();
        hs29SilkRollOff.reset();
    }

    inline double processInputAndInterstage(double x, double driveLin) noexcept
    {
        const double driven = x * driveLin;

        fluxHS56 = fluxPole * fluxHS56 + fluxScale * driven;
        const double dFlux = fluxHS56 - prevMagHS56;
        const double coreB = fluxHS56 + 0.14 * fastTanh(2.8 * dFlux);
        prevMagHS56 = 0.92 * prevMagHS56 + 0.08 * coreB;

        const double lfWarmth = 0.22 * fastTanh(coreB * 1.6) + 0.028 * coreB * std::abs(coreB);
        double y = x + lfWarmth / std::max(driveLin, 0.35);

        return hs29SilkRollOff.processSample(y);
    }

    inline double processOutputPeerless(double x, double driveLin) noexcept
    {
        const double driven = x * driveLin;
        fluxPeerless = fluxPole * fluxPeerless + fluxScale * driven;

        const double coreSat = fastTanh(driven * 0.56 + 0.26 * fluxPeerless) / 0.56;
        double y = (0.80 * driven + 0.20 * coreSat) / std::max(driveLin, 0.35);

        y = ironBassWeight.processSample(y);

        const double out = y - dcStateIn + dcBlockPole * dcStateOut;
        dcStateIn  = y;
        dcStateOut = out;
        return out;
    }

    double getTransformerContourDbAt(double freqHz) const noexcept
    {
        return ironBassWeight.getMagnitudeDbAt(freqHz, sampleRate)
             + hs29SilkRollOff.getMagnitudeDbAt(freqHz, sampleRate);
    }

private:
    double sampleRate = 48000.0;
    double tol = 0.0;
    double fluxPole = 0.995;
    double fluxScale = 0.005;
    double dcBlockPole = 0.999;

    double fluxHS56 = 0.0;
    double prevMagHS56 = 0.0;
    double fluxPeerless = 0.0;
    double dcStateIn = 0.0;
    double dcStateOut = 0.0;

    TPTFilter ironBassWeight;
    TPTFilter hs29SilkRollOff;
};

//==============================================================================
// 4. Push-Pull Vacuum Tube Amplifier (12AX7 + 12AU7 + 6X4 Rectifier Sag)
//==============================================================================
class PushPullTubeAmp_12AX7_12AU7
{
public:
    void prepare(double newSampleRate, double channelToleranceOffset = 0.0) noexcept
    {
        sampleRate = std::max(44100.0, newSampleRate);
        tol = channelToleranceOffset;

        biasAttCoeff = std::exp(-1.0 / (0.012 * sampleRate));
        biasRelCoeff = std::exp(-1.0 / (0.095 * sampleRate));

        sagAttCoeff = std::exp(-1.0 / (0.018 * sampleRate));
        sagRelCoeff = std::exp(-1.0 / (0.145 * sampleRate));

        const double fMiller = std::min(22000.0, sampleRate * 0.42);
        millerPole = std::exp(-kTwoPi * fMiller / sampleRate);

        reset();
    }

    void reset() noexcept
    {
        cathodeBiasShift = 0.0;
        psuSagEnv = 0.0;
        millerState = 0.0;
    }

    inline double processSample(double x, double tubeDriveLin, double &outHarmonicActivity) noexcept
    {
        const double vIn = x * tubeDriveLin;

        const double absIn = std::abs(vIn);
        if (absIn > cathodeBiasShift)
            cathodeBiasShift = biasAttCoeff * cathodeBiasShift + (1.0 - biasAttCoeff) * absIn;
        else
            cathodeBiasShift = biasRelCoeff * cathodeBiasShift + (1.0 - biasRelCoeff) * absIn;

        const double biasOffset = 0.090 * (1.0 + 0.05 * tol) + 0.040 * cathodeBiasShift;
        const double vGrid = vIn + biasOffset;

        double vPlate12AX7 = 0.0;
        if (vGrid >= 0.0)
        {
            const double gComp = vGrid / (1.0 + 0.22 * vGrid);
            vPlate12AX7 = fastTanh(gComp * 1.12) / 1.12;
        }
        else
        {
            vPlate12AX7 = fastTanh(vGrid * 0.94) / 0.94;
        }

        const double dcZero = fastTanh((biasOffset / (1.0 + 0.22 * biasOffset)) * 1.12) / 1.12;
        double stage1 = (vPlate12AX7 - dcZero) * (1.0 + 0.07 * biasOffset);

        millerState = millerPole * millerState + (1.0 - millerPole) * stage1;
        stage1 = millerState;

        const double powerDemand = stage1 * stage1;
        if (powerDemand > psuSagEnv)
            psuSagEnv = sagAttCoeff * psuSagEnv + (1.0 - sagAttCoeff) * powerDemand;
        else
            psuSagEnv = sagRelCoeff * psuSagEnv + (1.0 - sagRelCoeff) * powerDemand;

        const double bPlusHeadroom = 1.0 / (1.0 + 0.075 * psuSagEnv);

        const double posHalf = std::max(0.0, stage1);
        const double negHalf = std::min(0.0, stage1);

        const double gainTop = 1.008 + 0.010 * tol;
        const double gainBot = 0.992 - 0.010 * tol;

        const double pushTop = fastTanh(posHalf * gainTop * bPlusHeadroom) / bPlusHeadroom;
        const double pullBot = fastTanh(negHalf * gainBot * bPlusHeadroom) / bPlusHeadroom;

        const double rawPushPull = pushTop + pullBot;
        const double nfbMix = 0.74 * stage1 + 0.26 * rawPushPull;

        const double out = nfbMix / std::max(tubeDriveLin, 0.20);
        outHarmonicActivity = std::abs(out - x);
        return out;
    }

private:
    double sampleRate = 48000.0;
    double tol = 0.0;
    double biasAttCoeff = 0.99;
    double biasRelCoeff = 0.999;
    double sagAttCoeff = 0.99;
    double sagRelCoeff = 0.999;
    double millerPole = 0.18;

    double cathodeBiasShift = 0.0;
    double psuSagEnv = 0.0;
    double millerState = 0.0;
};

//==============================================================================
// 5. Polyphase Half-Band Allpass Oversampler (2x / 4x Zero-Aliasing Cascade)
//==============================================================================
class PolyphaseHalfBandStage
{
public:
    static constexpr std::array<double, 3> kA0 = { 0.0733894552636121, 0.3896948011510925, 0.7697418338632270 };
    static constexpr std::array<double, 3> kA1 = { 0.2194225464891439, 0.5845463218940122, 0.9238192845123890 };

    void reset() noexcept
    {
        x0.fill(0.0); y0.fill(0.0);
        x1.fill(0.0); y1.fill(0.0);
        z1 = 0.0;
    }

    inline void upsample(double input, double &outEven, double &outOdd) noexcept
    {
        double p0 = input;
        double p1 = input;
        for (size_t i = 0; i < 3; ++i)
        {
            const double n0 = kA0[i] * (p0 - y0[i]) + x0[i];
            x0[i] = p0; y0[i] = n0; p0 = n0;

            const double n1 = kA1[i] * (p1 - y1[i]) + x1[i];
            x1[i] = p1; y1[i] = n1; p1 = n1;
        }
        outEven = p0;
        outOdd  = p1;
    }

    inline double downsample(double inEven, double inOdd) noexcept
    {
        double p0 = inEven;
        double p1 = z1;
        z1 = inOdd;

        for (size_t i = 0; i < 3; ++i)
        {
            const double n0 = kA0[i] * (p0 - y0[i]) + x0[i];
            x0[i] = p0; y0[i] = n0; p0 = n0;

            const double n1 = kA1[i] * (p1 - y1[i]) + x1[i];
            x1[i] = p1; y1[i] = n1; p1 = n1;
        }
        return 0.5 * (p0 + p1);
    }

private:
    std::array<double, 3> x0{}, y0{}, x1{}, y1{};
    double z1 = 0.0;
};

//==============================================================================
// 6. Complete Single-Channel Pultec EQP-1A3 Physical Signal Path
//==============================================================================
class PultecChannelStrip
{
public:
    void prepare(double baseSampleRate, int oversampleOrder, double channelToleranceOffset) noexcept
    {
        baseFs = std::max(44100.0, baseSampleRate);
        osOrder = std::clamp(oversampleOrder, 0, 2);
        const double factor = (double)(1 << osOrder);
        oversampledFs = baseFs * factor;
        tolerance = channelToleranceOffset;

        passiveEQ.prepare(oversampledFs);
        transformers.prepare(oversampledFs, tolerance);
        tubeAmp.prepare(oversampledFs, tolerance);

        upStage1.reset();
        downStage1.reset();
        upStage2_A.reset();
        upStage2_B.reset();
        downStage2_A.reset();
        downStage2_B.reset();
    }

    void reset() noexcept
    {
        passiveEQ.reset();
        transformers.reset();
        tubeAmp.reset();
        upStage1.reset();
        downStage1.reset();
        upStage2_A.reset();
        upStage2_B.reset();
        downStage2_A.reset();
        downStage2_B.reset();
    }

    void setEQParameters(int lowFreqIdx, double lowBoost, double lowAtten,
                         int highBoostFreqIdx, double highBoost, double bandwidth,
                         int highAttenFreqIdx, double highAtten) noexcept
    {
        passiveEQ.setParameters(lowFreqIdx, lowBoost, lowAtten,
                                highBoostFreqIdx, highBoost, bandwidth,
                                highAttenFreqIdx, highAtten);
    }

    inline double processOversampledCore(double x, bool eqInCircuit, double tubeDriveLin,
                                         bool hardwareGainBoost, double &harmOut) noexcept
    {
        double sig = eqInCircuit ? passiveEQ.processSample(x) : x;
        sig = transformers.processInputAndInterstage(sig, tubeDriveLin);
        sig = tubeAmp.processSample(sig, tubeDriveLin, harmOut);
        sig = transformers.processOutputPeerless(sig, tubeDriveLin);

        if (hardwareGainBoost)
            sig *= 1.1389378; // +1.13 dB inherent hardware gain

        return sig;
    }

    inline double processSample(double x, bool eqInCircuit, double tubeDriveLin,
                                bool hardwareGainBoost, double &harmOut) noexcept
    {
        if (osOrder == 0)
        {
            return processOversampledCore(x, eqInCircuit, tubeDriveLin, hardwareGainBoost, harmOut);
        }
        else if (osOrder == 1)
        {
            double u0 = 0.0, u1 = 0.0;
            upStage1.upsample(x, u0, u1);
            double h0 = 0.0, h1 = 0.0;
            const double y0 = processOversampledCore(u0, eqInCircuit, tubeDriveLin, hardwareGainBoost, h0);
            const double y1 = processOversampledCore(u1, eqInCircuit, tubeDriveLin, hardwareGainBoost, h1);
            harmOut = 0.5 * (h0 + h1);
            return downStage1.downsample(y0, y1);
        }
        else
        {
            double u0 = 0.0, u1 = 0.0;
            upStage1.upsample(x, u0, u1);

            double u00 = 0.0, u01 = 0.0, u10 = 0.0, u11 = 0.0;
            upStage2_A.upsample(u0, u00, u01);
            upStage2_B.upsample(u1, u10, u11);

            double h0 = 0.0, h1 = 0.0, h2 = 0.0, h3 = 0.0;
            const double y00 = processOversampledCore(u00, eqInCircuit, tubeDriveLin, hardwareGainBoost, h0);
            const double y01 = processOversampledCore(u01, eqInCircuit, tubeDriveLin, hardwareGainBoost, h1);
            const double y10 = processOversampledCore(u10, eqInCircuit, tubeDriveLin, hardwareGainBoost, h2);
            const double y11 = processOversampledCore(u11, eqInCircuit, tubeDriveLin, hardwareGainBoost, h3);

            harmOut = 0.25 * (h0 + h1 + h2 + h3);
            const double d0 = downStage2_A.downsample(y00, y01);
            const double d1 = downStage2_B.downsample(y10, y11);
            return downStage1.downsample(d0, d1);
        }
    }

    double getMagnitudeDbAtFrequency(double freqHz, bool eqInCircuit) const noexcept
    {
        const double eqDb = eqInCircuit ? passiveEQ.getMagnitudeDbAtFrequency(freqHz) : 0.0;
        return eqDb + transformers.getTransformerContourDbAt(freqHz);
    }

private:
    double baseFs = 48000.0;
    double oversampledFs = 96000.0;
    int osOrder = 1;
    double tolerance = 0.0;

    CoupledPassivePultecNetwork passiveEQ;
    TriTransformerHysteresisEngine transformers;
    PushPullTubeAmp_12AX7_12AU7 tubeAmp;

    PolyphaseHalfBandStage upStage1, downStage1;
    PolyphaseHalfBandStage upStage2_A, upStage2_B, downStage2_A, downStage2_B;
};

//==============================================================================
// 7. Master Stereo / Mid-Side Pultec EQP-1A Ultra Engine (v2)
//==============================================================================
class PultecEQP1AMasterEngine
{
public:
    enum RoutingMode
    {
        RoutingStereo = 0,
        RoutingMidOnly = 1,
        RoutingSideOnly = 2,
        RoutingDualMonoAnalog = 3
    };

    void prepare(double sampleRate, int oversampleOrder = 1) noexcept
    {
        fs = std::max(44100.0, sampleRate);
        curOsOrder = std::clamp(oversampleOrder, 0, 2);

        chLeft.prepare(fs, curOsOrder, -0.75);
        chRight.prepare(fs, curOsOrder, +0.75);

        smoothCoeff = std::exp(-1.0 / (0.008 * fs));
        rmsCoeff    = std::exp(-1.0 / (0.180 * fs));

        inRmsEnv  = 1.0e-6;
        outRmsEnv = 1.0e-6;
        humPhase  = 0.0;
    }

    void reset() noexcept
    {
        chLeft.reset();
        chRight.reset();
        inRmsEnv  = 1.0e-6;
        outRmsEnv = 1.0e-6;
    }

    void startInputCalibration() noexcept
    {
        calPeakSq = 0.0;
        calSumSq  = 0.0;
        calSamples = 0;
        calWaitingForAudio.store(true);
        calPendingApply.store(false);
        calDisplayDone.store(false);
        calProgress.store(0.0f);
        calRecommendedDb.store(0.0f);
        calActive.store(true);
    }

    void cancelInputCalibration() noexcept
    {
        calActive.store(false);
        calWaitingForAudio.store(false);
        calPendingApply.store(false);
    }

    // Consumes the calibration result EXACTLY ONCE so the user can freely turn the Input knob afterwards!
    bool consumeCalibrationResult(float& outRecommendedDb) noexcept
    {
        if (calPendingApply.exchange(false, std::memory_order_acq_rel))
        {
            outRecommendedDb = calRecommendedDb.load(std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    bool isCalibrationActive() const noexcept       { return calActive.load(std::memory_order_relaxed); }
    bool isCalibrationWaiting() const noexcept      { return calWaitingForAudio.load(std::memory_order_relaxed); }
    bool isCalibrationDisplayDone() const noexcept  { return calDisplayDone.load(std::memory_order_relaxed); }
    float getCalibrationProgress() const noexcept   { return calProgress.load(std::memory_order_relaxed); }
    float getRecommendedInputDb() const noexcept    { return calRecommendedDb.load(std::memory_order_relaxed); }

    void updateParameters(float inputDb,
                          float outputDb,
                          float tubeDriveDb,
                          int lowFreqIdx,
                          float lowBoost,
                          float lowAtten,
                          int highBoostFreqIdx,
                          float highBoost,
                          float bandwidth,
                          int highAttenFreqIdx,
                          float highAtten,
                          bool eqIn,
                          bool trueBypass,
                          bool autoGain,
                          bool analogHum,
                          int routingMode,
                          int oversampleOrder) noexcept
    {
        if (oversampleOrder != curOsOrder)
        {
            prepare(fs, oversampleOrder);
        }

        targetInGain    = dbToLin((double)inputDb);
        targetOutGain   = dbToLin((double)outputDb);
        targetTubeDrive = dbToLin((double)tubeDriveDb);

        eqInCircuit  = eqIn;
        targetBypass = trueBypass ? 1.0 : 0.0;
        useAutoGain  = autoGain;
        useAnalogHum = analogHum;
        routing      = std::clamp(routingMode, 0, 3);

        chLeft.setEQParameters(lowFreqIdx, (double)lowBoost, (double)lowAtten,
                               highBoostFreqIdx, (double)highBoost, (double)bandwidth,
                               highAttenFreqIdx, (double)highAtten);

        const double tolScale = (routing == RoutingDualMonoAnalog) ? 1.012 : 1.0;
        chRight.setEQParameters(lowFreqIdx,
                                std::clamp((double)lowBoost * tolScale, 0.0, 10.0),
                                std::clamp((double)lowAtten / tolScale, 0.0, 10.0),
                                highBoostFreqIdx,
                                std::clamp((double)highBoost * tolScale, 0.0, 10.0),
                                std::clamp((double)bandwidth / tolScale, 0.0, 10.0),
                                highAttenFreqIdx,
                                std::clamp((double)highAtten * tolScale, 0.0, 10.0));
    }

    void processBlock(float* left, float* right, int numSamples) noexcept
    {
        double peakInBlock  = 0.0;
        double peakOutBlock = 0.0;
        double peakHarmBlock = 0.0;

        for (int i = 0; i < numSamples; ++i)
        {
            const double rawL = (double)left[i];
            const double rawR = right ? (double)right[i] : rawL;

            if (calActive.load(std::memory_order_relaxed))
            {
                const double monoEnergy = 0.5 * (rawL * rawL + rawR * rawR);
                if (calWaitingForAudio.load(std::memory_order_relaxed))
                {
                    if (monoEnergy > 1.0e-5)
                        calWaitingForAudio.store(false, std::memory_order_relaxed);
                }
                else
                {
                    calSumSq += monoEnergy;
                    if (monoEnergy > calPeakSq) calPeakSq = monoEnergy;
                    ++calSamples;

                    const int targetSamples = (int)(2.0 * fs);
                    calProgress.store(std::clamp((float)calSamples / (float)targetSamples, 0.0f, 1.0f),
                                      std::memory_order_relaxed);

                    if (calSamples >= targetSamples)
                    {
                        const double rmsDb  = 10.0 * std::log10(std::max(calSumSq / (double)calSamples, 1.0e-12));
                        const double peakDb = 10.0 * std::log10(std::max(calPeakSq, 1.0e-12));
                        const double recFromRms  = -14.0 - rmsDb;
                        const double recFromPeak = -4.5  - peakDb;
                        const double recDb = std::clamp(0.65 * recFromRms + 0.35 * recFromPeak, -18.0, 18.0);

                        calRecommendedDb.store((float)recDb, std::memory_order_relaxed);
                        calDisplayDone.store(true, std::memory_order_relaxed);
                        calPendingApply.store(true, std::memory_order_release);
                        calActive.store(false, std::memory_order_relaxed);
                    }
                }
            }

            curInGain    = smoothCoeff * curInGain    + (1.0 - smoothCoeff) * targetInGain;
            curOutGain   = smoothCoeff * curOutGain   + (1.0 - smoothCoeff) * targetOutGain;
            curTubeDrive = smoothCoeff * curTubeDrive + (1.0 - smoothCoeff) * targetTubeDrive;
            curBypass    = smoothCoeff * curBypass    + (1.0 - smoothCoeff) * targetBypass;

            double inL = rawL * curInGain;
            double inR = rawR * curInGain;

            peakInBlock = std::max(peakInBlock, std::max(std::abs(inL), std::abs(inR)));

            double procL = inL;
            double procR = inR;
            double harmL = 0.0, harmR = 0.0;

            const bool hwBoost = (routing == RoutingDualMonoAnalog) && !useAutoGain;

            if (routing == RoutingMidOnly)
            {
                const double mid  = 0.5 * (inL + inR);
                const double side = 0.5 * (inL - inR);
                const double midOut = chLeft.processSample(mid, eqInCircuit, curTubeDrive, hwBoost, harmL);
                procL = midOut + side;
                procR = midOut - side;
                harmR = harmL;
            }
            else if (routing == RoutingSideOnly)
            {
                const double mid  = 0.5 * (inL + inR);
                const double side = 0.5 * (inL - inR);
                const double sideOut = chRight.processSample(side, eqInCircuit, curTubeDrive, hwBoost, harmR);
                procL = mid + sideOut;
                procR = mid - sideOut;
                harmL = harmR;
            }
            else
            {
                procL = chLeft.processSample(inL, eqInCircuit, curTubeDrive, hwBoost, harmL);
                procR = chRight.processSample(inR, eqInCircuit, curTubeDrive, hwBoost, harmR);
            }

            const double inEnergy  = 0.5 * (inL * inL + inR * inR);
            const double outEnergy = 0.5 * (procL * procL + procR * procR);
            inRmsEnv  = rmsCoeff * inRmsEnv  + (1.0 - rmsCoeff) * inEnergy;
            outRmsEnv = rmsCoeff * outRmsEnv + (1.0 - rmsCoeff) * outEnergy;

            if (useAutoGain && eqInCircuit && inRmsEnv > 1.0e-7 && outRmsEnv > 1.0e-7)
            {
                const double ratio = std::sqrt(inRmsEnv / outRmsEnv);
                const double clampedComp = std::clamp(ratio, 0.25, 4.0);
                procL *= clampedComp;
                procR *= clampedComp;
            }

            if (useAnalogHum)
            {
                humPhase += kTwoPi * 60.0 / fs;
                if (humPhase >= kTwoPi) humPhase -= kTwoPi;
                const double hum = 0.000075 * (std::sin(humPhase) + 0.35 * std::sin(2.0 * humPhase));
                procL += hum;
                procR += hum;
            }

            procL *= curOutGain;
            procR *= curOutGain;

            const double finalL = (1.0 - curBypass) * procL + curBypass * rawL;
            const double finalR = (1.0 - curBypass) * procR + curBypass * rawR;

            left[i] = (float)finalL;
            if (right) right[i] = (float)finalR;

            peakOutBlock  = std::max(peakOutBlock, std::max(std::abs(finalL), std::abs(finalR)));
            peakHarmBlock = std::max(peakHarmBlock, 0.5 * (harmL + harmR));
        }

        meterInputPeak.store((float)peakInBlock, std::memory_order_relaxed);
        meterOutputPeak.store((float)peakOutBlock, std::memory_order_relaxed);
        meterTubeHarmonics.store((float)std::clamp(peakHarmBlock * 8.5, 0.0, 1.5), std::memory_order_relaxed);
    }

    double getMagnitudeDbAtFrequency(double freqHz) const noexcept
    {
        if (targetBypass > 0.5) return 0.0;
        return chLeft.getMagnitudeDbAtFrequency(freqHz, eqInCircuit);
    }

    float getInputMeter() const noexcept     { return meterInputPeak.load(std::memory_order_relaxed); }
    float getOutputMeter() const noexcept    { return meterOutputPeak.load(std::memory_order_relaxed); }
    float getHarmonicMeter() const noexcept  { return meterTubeHarmonics.load(std::memory_order_relaxed); }

private:
    double fs = 48000.0;
    int curOsOrder = 1;
    int routing = RoutingDualMonoAnalog;

    PultecChannelStrip chLeft;
    PultecChannelStrip chRight;

    double smoothCoeff = 0.995;
    double rmsCoeff    = 0.999;

    double curInGain = 1.0, targetInGain = 1.0;
    double curOutGain = 1.0, targetOutGain = 1.0;
    double curTubeDrive = 1.0, targetTubeDrive = 1.0;
    double curBypass = 0.0, targetBypass = 0.0;

    bool eqInCircuit  = true;
    bool useAutoGain  = false;
    bool useAnalogHum = false;

    double inRmsEnv  = 1.0e-6;
    double outRmsEnv = 1.0e-6;
    double humPhase  = 0.0;

    std::atomic<bool>  calActive{false};
    std::atomic<bool>  calWaitingForAudio{false};
    std::atomic<bool>  calPendingApply{false};
    std::atomic<bool>  calDisplayDone{false};
    std::atomic<float> calProgress{0.0f};
    std::atomic<float> calRecommendedDb{0.0f};
    double calPeakSq = 0.0;
    double calSumSq  = 0.0;
    int    calSamples = 0;

    std::atomic<float> meterInputPeak{0.0f};
    std::atomic<float> meterOutputPeak{0.0f};
    std::atomic<float> meterTubeHarmonics{0.0f};
};

} // namespace PultecDSP
