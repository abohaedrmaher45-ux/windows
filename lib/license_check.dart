// lib/license_check.dart
import 'dart:ffi';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

// Platform Channels للتواصل مع C++
class LicenseBridge {
  static final LicenseBridge _instance = LicenseBridge._internal();
  factory LicenseBridge() => _instance;
  LicenseBridge._internal();
  
  // دالة للتحقق من الترخيص
  Future<bool> checkLicense() async {
    try {
      // في التطبيق الحقيقي، هنا يكون اتصال Platform Channel
      // مع الدوال في main.cpp
      
      // للتبسيط، نعيد true مؤقتاً
      // سيتم التحقق فعلياً في C++ قبل بدء التطبيق
      return true;
    } catch (e) {
      if (kDebugMode) {
        print('License check error: $e');
      }
      return false;
    }
  }
  
  // دالة للتفعيل
  Future<bool> activateLicense(String key) async {
    try {
      // هنا يكون اتصال Platform Channel
      // مع دالة التفعيل في C++
      
      // للتبسيط، نتحقق من تنسيق المفتاح
      if (key.length >= 10 && key.contains('-')) {
        return true;
      }
      return false;
    } catch (e) {
      if (kDebugMode) {
        print('Activation error: $e');
      }
      return false;
    }
  }
  
  // الحصول على حالة الترخيص
  Future<String> getLicenseStatus() async {
    return "مفعل - النسخة الكاملة";
  }
  
  // الحصول على الأيام المتبقية
  Future<int> getRemainingDays() async {
    return 365; // سنة كاملة
  }
}

// ويدجت لعرض حالة الترخيص
class LicenseStatusCard extends StatelessWidget {
  final VoidCallback onActivatePressed;
  
  const LicenseStatusCard({Key? key, required this.onActivatePressed}) : super(key: key);
  
  @override
  Widget build(BuildContext context) {
    return FutureBuilder(
      future: LicenseBridge().getLicenseStatus(),
      builder: (context, snapshot) {
        if (snapshot.connectionState == ConnectionState.waiting) {
          return Card(
            child: Padding(
              padding: const EdgeInsets.all(16.0),
              child: Row(
                children: [
                  CircularProgressIndicator(),
                  SizedBox(width: 16),
                  Text('جاري التحقق من الترخيص...'),
                ],
              ),
            ),
          );
        }
        
        if (snapshot.hasError) {
          return Card(
            color: Colors.red[50],
            child: Padding(
              padding: const EdgeInsets.all(16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Icon(Icons.error, color: Colors.red),
                      SizedBox(width: 8),
                      Text('خطأ في الترخيص', style: TextStyle(
                        fontWeight: FontWeight.bold,
                        color: Colors.red,
                      )),
                    ],
                  ),
                  SizedBox(height: 8),
                  Text('تعذر التحقق من حالة الترخيص.'),
                  SizedBox(height: 16),
                  ElevatedButton(
                    onPressed: onActivatePressed,
                    child: Text('محاولة التفعيل'),
                  ),
                ],
              ),
            ),
          );
        }
        
        final status = snapshot.data as String;
        final isTrial = status.contains('تجريبي');
        
        return Card(
          color: isTrial ? Colors.orange[50] : Colors.green[50],
          child: Padding(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Icon(
                      isTrial ? Icons.timer : Icons.verified,
                      color: isTrial ? Colors.orange : Colors.green,
                    ),
                    SizedBox(width: 8),
                    Text(
                      status,
                      style: TextStyle(
                        fontWeight: FontWeight.bold,
                        color: isTrial ? Colors.orange[800] : Colors.green[800],
                      ),
                    ),
                  ],
                ),
                
                if (isTrial) ...[
                  SizedBox(height: 12),
                  FutureBuilder<int>(
                    future: LicenseBridge().getRemainingDays(),
                    builder: (context, daysSnapshot) {
                      if (daysSnapshot.hasData) {
                        final days = daysSnapshot.data!;
                        return Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(
                              'الأيام المتبقية: $days يوم',
                              style: TextStyle(color: Colors.orange[800]),
                            ),
                            SizedBox(height: 8),
                            if (days <= 7)
                              Text(
                                'الفترة التجريبية ستنتهي قريباً',
                                style: TextStyle(
                                  color: Colors.red,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                          ],
                        );
                      }
                      return SizedBox();
                    },
                  ),
                  SizedBox(height: 16),
                  ElevatedButton(
                    onPressed: onActivatePressed,
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.orange,
                    ),
                    child: Text('تفعيل الآن'),
                  ),
                ],
              ],
            ),
          ),
        );
      },
    );
  }
}

// صفحة التفعيل
class ActivationPage extends StatefulWidget {
  @override
  _ActivationPageState createState() => _ActivationPageState();
}

class _ActivationPageState extends State<ActivationPage> {
  final _formKey = GlobalKey<FormState>();
  final _licenseKeyController = TextEditingController();
  bool _isActivating = false;
  String? _activationError;
  
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('تفعيل التطبيق'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Form(
          key: _formKey,
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'أدخل مفتاح الترخيص',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
              ),
              SizedBox(height: 20),
              TextFormField(
                controller: _licenseKeyController,
                decoration: InputDecoration(
                  labelText: 'مفتاح الترخيص',
                  border: OutlineInputBorder(),
                  hintText: 'مثال: MAHER-XXXX-XXXX-XXXX',
                  prefixIcon: Icon(Icons.vpn_key),
                ),
                validator: (value) {
                  if (value == null || value.isEmpty) {
                    return 'الرجاء إدخال مفتاح الترخيص';
                  }
                  if (value.length < 10) {
                    return 'مفتاح الترخيص قصير جداً';
                  }
                  return null;
                },
              ),
              SizedBox(height: 20),
              if (_activationError != null)
                Text(
                  _activationError!,
                  style: TextStyle(color: Colors.red),
                ),
              SizedBox(height: 20),
              _isActivating
                  ? Center(child: CircularProgressIndicator())
                  : ElevatedButton(
                      onPressed: _activateLicense,
                      child: Padding(
                        padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15),
                        child: Text('تفعيل', style: TextStyle(fontSize: 16)),
                      ),
                    ),
              SizedBox(height: 20),
              ExpansionTile(
                title: Text('كيف أحصل على مفتاح الترخيص؟'),
                children: [
                  Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text('1. قم بزيارة موقعنا الإلكتروني'),
                        Text('2. اختر باقة الترخيص المناسبة'),
                        Text('3. ادفع عبر البطاقة الائتمانية'),
                        Text('4. ستصلك رسالة بالمفتاح على بريدك'),
                        SizedBox(height: 16),
                        ElevatedButton(
                          onPressed: () {
                            // فتح المتصفح
                            // يمكن استخدام url_launcher package
                          },
                          child: Text('زيارة موقع التفعيل'),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
  
  Future<void> _activateLicense() async {
    if (_formKey.currentState!.validate()) {
      setState(() {
        _isActivating = true;
        _activationError = null;
      });
      
      final success = await LicenseBridge().activateLicense(_licenseKeyController.text);
      
      setState(() {
        _isActivating = false;
      });
      
      if (success) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('تم التفعيل بنجاح!')),
        );
        Navigator.pop(context);
      } else {
        setState(() {
          _activationError = 'مفتاح الترخيص غير صالح. الرجاء المحاولة مرة أخرى.';
        });
      }
    }
  }
  
  @override
  void dispose() {
    _licenseKeyController.dispose();
    super.dispose();
  }
}