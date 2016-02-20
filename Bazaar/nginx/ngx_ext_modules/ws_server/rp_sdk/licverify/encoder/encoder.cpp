#include "rsa.h"
using CryptoPP::InvertibleRSAFunction;
using CryptoPP::RSAFunction;
using CryptoPP::RSA;
using CryptoPP::RSAES_OAEP_SHA_Encryptor;
using CryptoPP::RSAES_OAEP_SHA_Decryptor;
using CryptoPP::RSASS;

#include "pssr.h"
using CryptoPP::PSS;
using CryptoPP::PSSR;
#include "sha.h"
using CryptoPP::SHA1;
#include "osrng.h"
using CryptoPP::AutoSeededRandomPool;
#include "filters.h"
using CryptoPP::SignerFilter;
using CryptoPP::SignatureVerificationFilter;
using CryptoPP::StringSink;
using CryptoPP::ArraySink;
using CryptoPP::StringSource;
using CryptoPP::ArraySource;
using CryptoPP::PK_EncryptorFilter;
using CryptoPP::PK_DecryptorFilter;

#include <iostream>

#include "base64.h"
using CryptoPP::Base64Decoder;
using CryptoPP::Base64Encoder;
#include "base32.h"
using CryptoPP::Base32Decoder;
using CryptoPP::Base32Encoder;
#include "secblock.h"
using CryptoPP::SecByteBlock;
#include "cryptlib.h"
using CryptoPP::Exception;
using CryptoPP::DecodingResult;

#include "files.h"
using CryptoPP::FileSink;
using CryptoPP::FileSource;
using namespace std;

#include "encoder.h"
#include <iostream>
#include <fstream>

void SaveKey( const RSA::PublicKey& PublicKey, const string& filename )
{
    // DER Encode Key - X.509 key format
    PublicKey.Save(
        FileSink( filename.c_str(), true /*binary*/ ).Ref()
    );
}

void SaveKey( const RSA::PrivateKey& PrivateKey, const string& filename )
{
    // DER Encode Key - PKCS #8 key format
    PrivateKey.Save(
        FileSink( filename.c_str(), true /*binary*/ ).Ref()
    );
}

void LoadKey( const string& filename, RSA::PublicKey& PublicKey )
{
    // DER Encode Key - X.509 key format
    PublicKey.Load(
        FileSource( filename.c_str(), true, NULL, true /*binary*/ ).Ref()
    );
}

void LoadKey( const string& filename, RSA::PrivateKey& PrivateKey )
{
    // DER Encode Key - PKCS #8 key format
    PrivateKey.Load(
        FileSource( filename.c_str(), true, NULL, true /*binary*/ ).Ref()
    );
}

void GenerateKeyPair()
{
	// Generate keys
    AutoSeededRandomPool rng;

    InvertibleRSAFunction parameters;
    parameters.GenerateRandomWithKeySize( rng, 1024 );

    RSA::PrivateKey privateKey( parameters );
    RSA::PublicKey publicKey( parameters );

	SaveKey(privateKey, "priv.key");
	SaveKey(publicKey, "pub.key");
}

void GenerateKeyPair(std::string& public_key, std::string & private_key)
{
    std::string strprivkey, strpubkey;
    AutoSeededRandomPool rng;
    InvertibleRSAFunction privkey;

    privkey.Initialize(rng, 1024);

    Base64Encoder privkeysink(new StringSink(strprivkey), false);
    privkey.DEREncode(privkeysink);
    privkeysink.MessageEnd();

    RSAFunction pubkey(privkey);

    Base64Encoder pubkeysink(new StringSink(strpubkey), false);
    pubkey.DEREncode(pubkeysink);
    pubkeysink.MessageEnd();

    public_key = strpubkey;

    private_key = strprivkey;
}

std::string Encode(std::string _data)
{
	AutoSeededRandomPool rng;
	/*
	//Load keys from files
    RSA::PrivateKey privateKey;
    RSA::PublicKey publicKey;
	LoadKey("pubkey.txt", publicKey);
	LoadKey( "privkey.txt" , privateKey);
	*/

	std::string strprivate(_gPrivateKey);
    StringSource privString(strprivate, true, new Base64Decoder);

    // Signing
    RSASS<PSSR, SHA1>::Signer signer( privString );

    // Setup
    byte * message = (byte*)(_data.data());
    size_t messageLen = _data.size();
    ////////////////////////////////////////////////
    // Sign and Encode
    SecByteBlock signature(signer.MaxSignatureLength(messageLen));
    size_t signatureLen = signer.SignMessageWithRecovery(rng, message, messageLen, NULL, 0, signature);

	string encoded;

	StringSource ss(signature.data(), signatureLen, true,
		new Base32Encoder(
			new StringSink(encoded)
		) // Base64Encoder
	); // StringSource

	return encoded;
}

std::string Decode(std::string _encoded_data)
{
    ofstream myfile;
    myfile.open ("/tmp/decoder.log", ios::out | ios::app);
    myfile << "----------------------------------------" << endl;
    myfile << "_encoded_data: " << _encoded_data << endl;
	AutoSeededRandomPool rng;
	/*
    RSA::PublicKey publicKey;
	LoadKey("pubkey.txt", publicKey);
	*/
	std::string strpublic(_gPublicKey);
    myfile << "strpublic: " << strpublic << endl;

   StringSource pubString(strpublic, true, new Base64Decoder);

    RSASS<PSSR, SHA1>::Verifier verifier( pubString );

    std::array<unsigned char, 4096> decoded;
    ArraySink* as = new ArraySink(decoded.data(), 4096);

	ArraySource ss1(_encoded_data, true,
		new Base32Decoder(as) // Base64Decoder
	); // StringSource

	int signatureLen = decoded.size();
    myfile << "Decoded len " << signatureLen << endl;
    myfile << "AvaliableSize len " << as->AvaliableSize() << endl;
    myfile << "TotalPutlen len " << as->TotalPutLength() << endl;
    //myfile << "Decoded: " << decoded << endl;

    ////////////////////////////////////////////////
    // Verify and Recover
    SecByteBlock recovered(verifier.MaxRecoverableLengthFromSignatureLength(signatureLen));

	SecByteBlock b1((unsigned char*)decoded.data(), decoded.size());
    DecodingResult result = verifier.RecoverMessage(recovered, NULL,
            0, b1, signatureLen);
    myfile << "== BEFORE IF ==" << endl;

    if (!result.isValidCoding) {
        myfile << "Exc: Invalid Signature" << endl;
        return "";
        //throw Exception( Exception::OTHER_ERROR, "Invalid Signature" );
    }

	string res((char*)recovered.data(), recovered.size());
    myfile << "Res: " << res << endl;
    myfile.close();

	return res;
}
