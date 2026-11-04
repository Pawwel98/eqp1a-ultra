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
// 1. Topology-Preserving Transform (TPT / Zero-Delay Feedback) State-Variable Filter
//    Models analog RC shelves and RLC inductor tank circuits without Nyquist cramping
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
        cutoffHz = std::clamp(cutoffHz, 10.0, sampleRate * 0.46);
        Q = std::clamp(Q, 0.2, 10.0);
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
        cutoffHz = std::clamp(cutoffHz, 40.0, sampleRate * 0.46);
        Q = std::clamp(Q, 0.2, 10.0);
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
        centerHz = std::clamp(centerHz, 20.0, sampleRate * 0.46);
        Q = std::clamp(Q, 0.18, 18.0);
        const double A = std::pow(10.0, gainDb / 40.0);
        g = std::tan(kPi * centerHz / sampleRate);
        k = 1.0 / (Q * A);
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

    // Exact magnitude squared |H(e^{jw})|^2 for real-time analyzer curve
    double getMagnitudeDbAt(double freqHz, double sampleRate) const noexcept
    {
        if (freqHz <= 1.0 || freqHz >= sampleRate * 0.49) return 0.0;
        const double w = std::tan(kPi * freqHz / sampleRate);
        const double s = w / std::max(g, 1.0e-9);
        const double s2 = s * s;
        // Continuous-time mapped SVF prototype:
        // H(s) = m0 + m1 * (g*s)/(s^2 + k*s + 1) ? Wait: in TPT SVF:
        // V1/V0 = (s) / (s^2 + k*s + 1), V2/V0 = 1 / (s^2 + k*s + 1)
        const double denR = 1.0 - s2;
        const double denI = k * s;
        const double numR = m0 * (1.0 - s2) + m2;
        const double numI = (m0 * k + m1) * s;
        const double mag2 = (numR * numR + numI * numI) / std::max(denR * denR + denI * denI, 1.0e-18);
        return 10.0 * std::log10(std::max(mag2, 1.0e-12));
    }
};

//==============================================================================
// 2. Coupled Passive LCR Filter Network (Pultec EQP-1A3 Topology)
//    - Low Freq Selector: 20, 30, 45 (Modern), 60, 100, 150 (Modern) CPS
//    - Low Boost (10k Audio Pot, +13.5 dB) & Low Atten (100k Audio Pot, -17.5 dB)
//      with real asymmetric capacitor turnover offset + mutual bridge loading dip
//    - High Boost Selector: 3, 4, 5, 8, 10, 12, 16, 20 (Air) KCS
//      Multi-tapped toroidal inductor (150 mH -> 27 mH) with non-linear flux saturation
//      and Bandwidth (Q-damping resistor) interaction (+18 dB sharp -> +9.2 dB broad)
//    - High Atten Selector (ATTEN SEL): 5, 10, 20 KCS (-16 dB passive RC shelf)
//==============================================================================
class CoupledPassivePultecNetwork
{
public:
    static constexpr std::array<double, 6> kLowFreqsHz  = { 20.0, 30.0, 45.0, 60.0, 100.0, 150.0 };
    static constexpr std::array<double, 8> kHighBoostHz = { 3000.0, 4000.0, 5000.0, 8000.0, 10000.0, 12000.0, 16000.0, 20000.0 };
    static constexpr std::array<double, 3> kHighAttenHz = { 5000.0, 10000.0, 20000.0 };

    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(44100.0, newSampleRate);
        reset();
        updateCoefficients();
    }

    void reset() noexcept
    {
        lowBoostShelf.reset();
        lowAttenShelf.reset();
        lowMutualDipBell.reset();
        highBoostInductorBell.reset();
        highAttenShelf.reset();
        highMutualCouplingBell.reset();
        inductorFluxState = 0.0;
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
        const int clampedLowIdx  = std::clamp(lowFreqIdx, 0, (int)kLowFreqsHz.size() - 1);
        const int clampedHBIdx   = std::clamp(highBoostFreqIdx, 0, (int)kHighBoostHz.size() - 1);
        const int clampedHAIdx   = std::clamp(highAttenFreqIdx, 0, (int)kHighAttenHz.size() - 1);

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
        const double fLowMarked = kLowFreqsHz[(size_t)curLowIdx];

        // 1. LOW FREQUENCY PASSIVE RC NETWORK
        // In the original EQP-1A schematic, the Low Boost (10k log pot, e.g. 0.82uF @ 30Hz)
        // and Low Atten (100k log pot, e.g. 0.027uF @ 30Hz) share the selector switch,
        // but their 3dB turnover corners sit wide apart:
        // Boost shelf corner reaches up ~2.6x above the marked dial frequency,
        // while Attenuation shelf corner sits ~4.1x above the marked dial frequency (~0.65 octaves higher).
        const double lbNorm = curLowBoost * 0.1;
        const double laNorm = curLowAtten * 0.1;

        // Audio-taper (logarithmic resistance curve) combined with passive insertion divider
        const double lbTaper = std::pow(lbNorm, 1.32);
        const double laTaper = std::pow(laNorm, 1.25);

        // Mutual bridge loading: when both Boost and Atten pots are raised,
        // the 10k boost divider loads the 100k attenuation bridge.
        const double mutualLowLoading = lbTaper * laTaper;

        const double lowBoostDb = 13.5 * lbTaper * (1.0 - 0.11 * laTaper);
        const double lowAttenDb = -17.5 * laTaper * (1.0 - 0.07 * lbTaper);

        // Passive constant-bandwidth behavior: corner slides slightly down as boost increases
        const double fBoostCorner = fLowMarked * (2.85 - 0.55 * lbTaper - 0.22 * mutualLowLoading);
        const double fAttenCorner = fLowMarked * (4.45 - 0.40 * laTaper + 0.35 * mutualLowLoading);

        const double qBoost = 0.54 + 0.14 * lbTaper + 0.08 * mutualLowLoading;
        const double qAtten = 0.50 + 0.12 * laTaper;

        lowBoostShelf.setLowShelf(sampleRate, fBoostCorner, qBoost, lowBoostDb);
        lowAttenShelf.setLowShelf(sampleRate, fAttenCorner, qAtten, lowAttenDb);

        // Interactive bridge impedance dip ("Pultec Low-End Trick" low-mid phase/impedance pocket)
        // Centers around 3.6x - 5.2x the selected CPS tap when both pots are engaged
        const double dipCenterHz = fLowMarked * (4.1 + 0.9 * (laTaper - lbTaper));
        const double dipGainDb   = -3.4 * std::pow(mutualLowLoading, 0.78);
        const double dipQ        = 0.85 + 0.65 * mutualLowLoading;
        lowMutualDipBell.setBell(sampleRate, dipCenterHz, dipQ, dipGainDb);

        // 2. HIGH FREQUENCY RESONANT LCR INDUCTOR BOOST NETWORK
        // Multi-tapped toroidal coil (150 mH @ 3kHz down to 27 mH @ 16kHz).
        // Bandwidth control adjusts series resistor R_bw across the LCR tank:
        // Sharp (0.0) -> Max boost +18.0 dB, higher Q
        // Broad (10.0) -> Max boost +9.2 dB, wide musical skirt (Q down to 0.38)
        const double fHighBoost = kHighBoostHz[(size_t)curHBIdx];
        const double hbNorm     = curHighBoost * 0.1;
        const double bwNorm     = curBandwidth * 0.1; // 0 = Sharp, 1 = Broad

        const double hbTaper    = std::pow(hbNorm, 1.22);
        const double maxHighGainAtBw = 18.0 - 8.8 * std::pow(bwNorm, 0.82);
        const double highBoostDb     = maxHighGainAtBw * hbTaper;

        // Constant-bandwidth passive inductor Q: narrows as boost is pushed, widens as Bandwidth increases
        const double baseQ = 2.45 * std::pow(0.175, bwNorm); // ~2.45 at Sharp -> ~0.43 at Broad
        const double effectiveQ = baseQ * (0.62 + 0.58 * hbTaper);

        highBoostInductorBell.setBell(sampleRate, fHighBoost, effectiveQ, highBoostDb);
        inductorDriveAmount = hbTaper * (1.15 - 0.35 * bwNorm);

        // 3. HIGH FREQUENCY PASSIVE RC ATTENUATION SHELF (ATTEN SEL: 5, 10, 20 KCS)
        const double fHighAtten = kHighAttenHz[(size_t)curHAIdx];
        const double haNorm     = curHighAtten * 0.1;
        const double haTaper    = std::pow(haNorm, 1.24);
        const double highAttenDb = -16.0 * haTaper;

        // Passive shelf turnover sits slightly below marked KCS so 20 KCS gently rolls off down to 8-10 kHz
        const double fHighAttenCorner = fHighAtten * (0.72 - 0.10 * haTaper);
        highAttenShelf.setHighShelf(sampleRate, fHighAttenCorner, 0.56 + 0.10 * haTaper, highAttenDb);

        // High-band passive loading interaction when both High Boost and High Atten are active
        const double highMutual = hbTaper * haTaper;
        const double fCoupling  = std::sqrt(fHighBoost * fHighAtten);
        highMutualCouplingBell.setBell(sampleRate, fCoupling, 0.75, -1.65 * highMutual);
    }

    inline double processSample(double x) noexcept
    {
        // Low passive ladder
        double y = lowBoostShelf.processSample(x);
        y = lowAttenShelf.processSample(y);
        y = lowMutualDipBell.processSample(y);

        // High RLC inductor boost with subtle toroidal core permeability saturation on transients
        double beforeInductor = y;
        double afterInductor  = highBoostInductorBell.processSample(beforeInductor);
        double inductorDelta  = afterInductor - beforeInductor;

        if (inductorDriveAmount > 0.001)
        {
            // Toroidal permalloy core magnetic saturation on resonant HF current
            inductorFluxState = 0.96 * inductorFluxState + 0.04 * inductorDelta;
            const double satDelta = fastTanh(inductorDelta * (1.0 + 0.42 * inductorDriveAmount))
                                  / (1.0 + 0.36 * inductorDriveAmount);
            y = beforeInductor + satDelta - 0.018 * inductorDriveAmount * inductorFluxState * inductorFluxState;
        }
        else
        {
            y = afterInductor;
        }

        // High passive RC attenuation shelf
        y = highAttenShelf.processSample(y);
        y = highMutualCouplingBell.processSample(y);
        return y;
    }

    double getMagnitudeDbAtFrequency(double freqHz) const noexcept
    {
        return lowBoostShelf.getMagnitudeDbAt(freqHz, sampleRate)
             + lowAttenShelf.getMagnitudeDbAt(freqHz, sampleRate)
             + lowMutualDipBell.getMagnitudeDbAt(freqHz, sampleRate)
             + highBoostInductorBell.getMagnitudeDbAt(freqHz, sampleRate)
             + highAttenShelf.getMagnitudeDbAt(freqHz, sampleRate)
             + highMutualCouplingBell.getMagnitudeDbAt(freqHz, sampleRate);
    }

private:
    double sampleRate = 48000.0;
    int curLowIdx = -1, curHBIdx = -1, curHAIdx = -1;
    double curLowBoost = -1.0, curLowAtten = -1.0;
    double curHighBoost = -1.0, curBandwidth = -1.0, curHighAtten = -1.0;
    double inductorDriveAmount = 0.0;
    double inductorFluxState = 0.0;

    TPTFilter lowBoostShelf;
    TPTFilter lowAttenShelf;
    TPTFilter lowMutualDipBell;
    TPTFilter highBoostInductorBell;
    TPTFilter highAttenShelf;
    TPTFilter highMutualCouplingBell;
};

//==============================================================================
// 3. Three-Transformer Magnetic Hysteresis & Core Saturation Engine
//    Models:
//      1) Triad HS-56 Input Transformer (LF magnetic flux B-H loop saturation)
//      2) Triad HS-29 Interstage Transformer (high-Z eddy-current & 19.5 kHz air)
//      3) Peerless S-217-D Output Transformer (negative-feedback iron weight)
//==============================================================================
class TriTransformerHysteresisEngine
{
public:
    void prepare(double newSampleRate, double channelToleranceOffset = 0.0) noexcept
    {
        sampleRate = std::max(44100.0, newSampleRate);
        tol = channelToleranceOffset; // Slight L/R analog hardware variance

        // Magnetic flux leaky integrator cutoff (~42 Hz core saturation corner)
        const double fFlux = 42.0 * (1.0 + 0.03 * tol);
        fluxPole = std::exp(-kTwoPi * fFlux / sampleRate);
        fluxScale = (1.0 - fluxPole);

        // Sub-sonic DC / transformer blocking filter (7.5 Hz head bump contour)
        dcBlockPole = std::exp(-kTwoPi * 7.5 / sampleRate);

        // Interstage Triad HS-29 subtle winding resonance (+0.45 dB @ 18.8 kHz for Pultec silk)
        hs29AirResonance.setBell(sampleRate, 18800.0 * (1.0 + 0.02 * tol), 0.72, 0.48);

        // Peerless S-217-D low-end transformer head-bump (+0.55 dB @ 48 Hz)
        peerlessHeadBump.setBell(sampleRate, 48.0 * (1.0 + 0.02 * tol), 0.68, 0.55);

        reset();
    }

    void reset() noexcept
    {
        fluxHS56 = 0.0;
        prevMagHS56 = 0.0;
        fluxPeerless = 0.0;
        dcStateIn = 0.0;
        dcStateOut = 0.0;
        hs29AirResonance.reset();
        peerlessHeadBump.reset();
    }

    // Stage 1: Input Triad HS-56 + Interstage Triad HS-29 (before tube stage)
    inline double processInputAndInterstage(double x, double driveLin) noexcept
    {
        const double driven = x * driveLin;

        // Magnetic flux B(t) = integral of V(t) dt -> naturally stronger at low frequencies (20-80 Hz)
        fluxHS56 = fluxPole * fluxHS56 + fluxScale * driven;

        // Jiles-Atherton / Langevin inspired hysteresis with coercive pinning
        const double dFlux = fluxHS56 - prevMagHS56;
        const double hystPin = 0.14 * fastTanh(3.2 * dFlux);
        const double coreB = fluxHS56 + hystPin;
        prevMagHS56 = 0.92 * prevMagHS56 + 0.08 * coreB;

        // Core saturation generates rich low-frequency 2nd & 3rd harmonics without fizzing highs
        const double lfHarmonics = (coreB - fastTanh(coreB * 1.45) / 1.45);
        const double asymCore = 0.028 * (1.0 + 0.15 * tol) * coreB * std::abs(coreB);

        double y = x - (0.34 * lfHarmonics - asymCore) / std::max(driveLin, 0.25);

        // Triad HS-29 interstage winding silk
        y = hs29AirResonance.processSample(y);
        return y;
    }

    // Stage 2: Output Peerless S-217-D (after 12AU7 push-pull tube stage)
    inline double processOutputPeerless(double x, double driveLin) noexcept
    {
        const double driven = x * driveLin;
        fluxPeerless = fluxPole * fluxPeerless + fluxScale * driven;

        // Peerless S-217-D is inside the negative feedback loop, giving ultra-tight LF punch
        const double coreSat = fastTanh(driven * 0.65 + 0.22 * fluxPeerless) / 0.65;
        double y = (0.78 * driven + 0.22 * coreSat) / std::max(driveLin, 0.25);

        // Authentic 48 Hz transformer resonance bump
        y = peerlessHeadBump.processSample(y);

        // Clean transformer DC blocking
        const double out = y - dcStateIn + dcBlockPole * dcStateOut;
        dcStateIn = y;
        dcStateOut = out;
        return out;
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

    TPTFilter hs29AirResonance;
    TPTFilter peerlessHeadBump;
};

//==============================================================================
// 4. Push-Pull Vacuum Tube Amplifier (12AX7 Voltage Amp + 12AU7 Push-Pull + 6X4 Sag)
//    - Restores the 20 dB passive filter loss (+1.13 dB inherent hardware gain)
//    - Produces smooth even-order (2nd harmonic) warmth from the 12AX7 stage
//      and balanced push-pull density from the 12AU7 stage
//==============================================================================
class PushPullTubeAmp_12AX7_12AU7
{
public:
    void prepare(double newSampleRate, double channelToleranceOffset = 0.0) noexcept
    {
        sampleRate = std::max(44100.0, newSampleRate);
        tol = channelToleranceOffset;

        // Cathode bypass capacitor bias shift (time constant ~12 ms attack, ~95 ms release)
        biasAttCoeff = std::exp(-1.0 / (0.012 * sampleRate));
        biasRelCoeff = std::exp(-1.0 / (0.095 * sampleRate));

        // 6X4 vacuum rectifier power supply sag (18 ms attack, 145 ms recovery)
        sagAttCoeff = std::exp(-1.0 / (0.018 * sampleRate));
        sagRelCoeff = std::exp(-1.0 / (0.145 * sampleRate));

        // Miller capacitance HF smoothing across 12AX7 plate (~36 kHz roll-off at oversampled rate)
        const double fMiller = std::min(34000.0, sampleRate * 0.44);
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

        // 1. 12AX7 / ECC83 Single-Ended Triode Stage (Asymmetric -> Rich 2nd Harmonic H2)
        // Grid conduction & dynamic cathode bias shift
        const double absIn = std::abs(vIn);
        if (absIn > cathodeBiasShift)
            cathodeBiasShift = biasAttCoeff * cathodeBiasShift + (1.0 - biasAttCoeff) * absIn;
        else
            cathodeBiasShift = biasRelCoeff * cathodeBiasShift + (1.0 - biasRelCoeff) * absIn;

        const double biasOffset = 0.095 * (1.0 + 0.06 * tol) + 0.045 * cathodeBiasShift;
        const double vGrid = vIn + biasOffset;

        // Asymmetric triode transfer curve (positive half-cycle compresses softly via grid current;
        // negative half-cycle follows 3/2-power Child-Langmuir space-charge law)
        double vPlate12AX7 = 0.0;
        if (vGrid >= 0.0)
        {
            const double gComp = vGrid / (1.0 + 0.24 * vGrid);
            vPlate12AX7 = fastTanh(gComp * 1.18) / 1.18;
        }
        else
        {
            vPlate12AX7 = fastTanh(vGrid * 0.96) / 0.96;
        }

        // Remove static DC offset introduced by biasOffset at zero signal
        const double dcZero = fastTanh((biasOffset / (1.0 + 0.24 * biasOffset)) * 1.18) / 1.18;
        double stage1 = (vPlate12AX7 - dcZero) * (1.0 + 0.08 * biasOffset);

        // Miller capacitance smoothing
        millerState = millerPole * millerState + (1.0 - millerPole) * stage1;
        stage1 = millerState;

        // 2. 6X4 Tube Rectifier B+ Rail Sag
        const double powerDemand = stage1 * stage1;
        if (powerDemand > psuSagEnv)
            psuSagEnv = sagAttCoeff * psuSagEnv + (1.0 - sagAttCoeff) * powerDemand;
        else
            psuSagEnv = sagRelCoeff * psuSagEnv + (1.0 - sagRelCoeff) * powerDemand;

        const double bPlusHeadroom = 1.0 / (1.0 + 0.085 * psuSagEnv);

        // 3. 12AU7 / ECC82 Push-Pull Output Stage with Negative Feedback
        // Matched pair with realistic 1.8% vintage triode transconductance asymmetry
        const double posHalf = std::max(0.0, stage1);
        const double negHalf = std::min(0.0, stage1);

        const double gainTop = 1.009 + 0.012 * tol;
        const double gainBot = 0.991 - 0.012 * tol;

        const double pushTop = fastTanh(posHalf * gainTop * bPlusHeadroom) / bPlusHeadroom;
        const double pullBot = fastTanh(negHalf * gainBot * bPlusHeadroom) / bPlusHeadroom;

        // Negative feedback from Peerless S-217-D secondary linearizes the stage while keeping musical harmonics
        const double rawPushPull = pushTop + pullBot;
        const double nfbMix = 0.72 * stage1 + 0.28 * rawPushPull;

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
    double millerPole = 0.15;

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
    // 12th-order polyphase allpass coefficients for >98 dB stopband attenuation
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
        osOrder = std::clamp(oversampleOrder, 0, 2); // 0=1x, 1=2x, 2=4x
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
        // 1. Passive LCR Filter Network (bypassed when EQ IN switch is OFF, just like real hardware!)
        double sig = eqInCircuit ? passiveEQ.processSample(x) : x;

        // 2. Triad HS-56 Input + Triad HS-29 Interstage Transformers
        sig = transformers.processInputAndInterstage(sig, tubeDriveLin);

        // 3. 12AX7 + 12AU7 Push-Pull Vacuum Tube Make-Up Amplifier
        sig = tubeAmp.processSample(sig, tubeDriveLin, harmOut);

        // 4. Peerless S-217-D Output Transformer
        sig = transformers.processOutputPeerless(sig, tubeDriveLin);

        // 5. Inherent +1.13 dB Pultec EQP-1A insertion boost (when enabled in Vintage mode)
        if (hardwareGainBoost)
            sig *= 1.1389378; // +1.13 dB

        return sig;
    }

    inline double processSample(double x, bool eqInCircuit, double tubeDriveLin,
                                bool hardwareGainBoost, double &harmOut) noexcept
    {
        if (osOrder == 0)
        {
            return processOversampledCore(x, eqInCircuit, tubeDriveLin, hardwareGainBoost, harmOut);
        }
        else if (osOrder == 1) // 2x Oversampling
        {
            double u0 = 0.0, u1 = 0.0;
            upStage1.upsample(x, u0, u1);
            double h0 = 0.0, h1 = 0.0;
            const double y0 = processOversampledCore(u0, eqInCircuit, tubeDriveLin, hardwareGainBoost, h0);
            const double y1 = processOversampledCore(u1, eqInCircuit, tubeDriveLin, hardwareGainBoost, h1);
            harmOut = 0.5 * (h0 + h1);
            return downStage1.downsample(y0, y1);
        }
        else // 4x Oversampling
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

    double getMagnitudeDbAtFrequency(double freqHz) const noexcept
    {
        return passiveEQ.getMagnitudeDbAtFrequency(freqHz);
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
// 7. Master Stereo / Mid-Side Pultec EQP-1A Ultra Engine
//    Includes:
//    - Stereo (L/R), Mid-Only, Side-Only, and Dual-Mono Analog Tolerance modes
//    - Auto-Gain loudness matching
//    - Shift+LMB Smart Input Auto-Calibration (2.2s drum/mix detector)
//    - Authentic 60 Hz tube heater hum generator (switchable)
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

        // Channel 0 and Channel 1 with subtle realistic hardware component tolerance
        chLeft.prepare(fs, curOsOrder, -0.85);
        chRight.prepare(fs, curOsOrder, +0.85);

        smoothCoeff = std::exp(-1.0 / (0.008 * fs)); // 8 ms click-free smoothing
        rmsCoeff    = std::exp(-1.0 / (0.180 * fs)); // 180 ms Auto-Gain RMS detector

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

    // Shift + LMB Input Auto-Calibration API (same workflow as CERVINIZATOR)
    void startInputCalibration() noexcept
    {
        calPeakSq = 0.0;
        calSumSq  = 0.0;
        calSamples = 0;
        calWaitingForAudio.store(true);
        calCompleted.store(false);
        calProgress.store(0.0f);
        calRecommendedDb.store(0.0f);
        calActive.store(true);
    }

    void cancelInputCalibration() noexcept
    {
        calActive.store(false);
        calWaitingForAudio.store(false);
    }

    bool isCalibrationActive() const noexcept       { return calActive.load(); }
    bool isCalibrationWaiting() const noexcept      { return calWaitingForAudio.load(); }
    bool isCalibrationComplete() const noexcept     { return calCompleted.load(); }
    float getCalibrationProgress() const noexcept   { return calProgress.load(); }
    float getRecommendedInputDb() const noexcept    { return calRecommendedDb.load(); }

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

        // In Dual-Mono Analog mode, right channel has realistic 1.5% vintage potentiometer tolerance
        const double tolScale = (routing == RoutingDualMonoAnalog) ? 1.015 : 1.0;
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

            // --- Shift+LMB Input Calibration Detector ---
            if (calActive.load(std::memory_order_relaxed))
            {
                const double monoEnergy = 0.5 * (rawL * rawL + rawR * rawR);
                if (calWaitingForAudio.load(std::memory_order_relaxed))
                {
                    if (monoEnergy > 1.0e-5) // > -50 dBFS threshold
                        calWaitingForAudio.store(false, std::memory_order_relaxed);
                }
                else
                {
                    calSumSq += monoEnergy;
                    if (monoEnergy > calPeakSq) calPeakSq = monoEnergy;
                    ++calSamples;

                    const int targetSamples = (int)(2.2 * fs);
                    calProgress.store(std::clamp((float)calSamples / (float)targetSamples, 0.0f, 1.0f),
                                      std::memory_order_relaxed);

                    if (calSamples >= targetSamples)
                    {
                        const double rmsDb  = 10.0 * std::log10(std::max(calSumSq / (double)calSamples, 1.0e-12));
                        const double peakDb = 10.0 * std::log10(std::max(calPeakSq, 1.0e-12));
                        // Target optimal Pultec transformer/tube sweet spot: -14 dBFS RMS / -4.5 dBFS Peak
                        const double recFromRms  = -14.0 - rmsDb;
                        const double recFromPeak = -4.5  - peakDb;
                        const double recDb = std::clamp(0.65 * recFromRms + 0.35 * recFromPeak, -18.0, 18.0);

                        calRecommendedDb.store((float)recDb, std::memory_order_relaxed);
                        calCompleted.store(true, std::memory_order_relaxed);
                        calActive.store(false, std::memory_order_relaxed);
                    }
                }
            }

            // Smooth parameters
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

            // Auto-Gain Spectral Loudness Compensation
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

            // Optional 60 Hz 6X4 power supply tube heater hum (-82 dBFS subtle analog floor)
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

            // Click-free True Bypass crossfade
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
        if (!eqInCircuit || targetBypass > 0.5) return 0.0;
        return chLeft.getMagnitudeDbAtFrequency(freqHz);
    }

    float getInputMeter() const noexcept     { return meterInputPeak.load(std::memory_order_relaxed); }
    float getOutputMeter() const noexcept    { return meterOutputPeak.load(std::memory_order_relaxed); }
    float getHarmonicMeter() const noexcept  { return meterTubeHarmonics.load(std::memory_order_relaxed); }

private:
    double fs = 48000.0;
    int curOsOrder = 1;
    int routing = RoutingStereo;

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

    // Input Auto-Calibration state
    std::atomic<bool>  calActive{false};
    std::atomic<bool>  calWaitingForAudio{false};
    std::atomic<bool>  calCompleted{false};
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
